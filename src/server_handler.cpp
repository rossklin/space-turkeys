#include <iostream>
#include <fstream>

#include <chrono>  // chrono::system_clock
#include <ctime>   // localtime
#include <sstream> // stringstream
#include <iomanip> // put_time
#include <string>  // string

#include "game_handler.h"
#include "game_data.h"
#include "server_handler.h"
#include "serialization.h"
#include "protocol.h"

using namespace std;
using namespace st3;
using namespace server;

// show server handler output even though DEBUG is not defined
void local_output(string v) {
  output(v, true);
}

bool valid_string(string v) {
  // todo
  return v.length() < 1000;
}

void end_thread(thread *&t) {
  t -> join();
  delete t;
  t = 0;
}

string current_time_and_date() {
  auto now = chrono::system_clock::now();
  auto in_time_t = chrono::system_clock::to_time_t(now);

  stringstream ss;
  ss << put_time(localtime(&in_time_t), "%Y-%m-%d %X");
  return ss.str();
}

void handler::log(string v) {
  static mutex m;
  string log_file = "server.log";

  m.lock();
  fstream f(log_file, ios::app);

  if (!f.is_open()) {
    cerr << "Failed to open log file!" << endl;
    throw runtime_error("Failed to open log file!");
  }

  f << current_time_and_date() << ": " << v << endl;
  f.close();
  
  m.unlock();
}

com *handler::access_game(string gid, bool do_lock) {
  com *g = 0;
  if (do_lock) game_ring.lock();
  if (games.count(gid)) {
    g = &games[gid];
  }
  if (do_lock) game_ring.unlock();
  return g;
}

com *handler::create_game(string gid, client_game_settings set, bool do_lock) {
  com *g;
  if (do_lock) game_ring.lock();
  if (games.count(gid)) {
    throw runtime_error("Attempted to create game but id alread in use!");
  } else {
    g = &games[gid];
    static_cast<client_game_settings&>(g -> settings) = set;
  }
  if (do_lock) game_ring.unlock();
  return g;
}

void handler::wfg(client_t *c) {
  query_handler handler = [] (int cid, sf::Packet p) -> handler_result {
    handler_result res;
    res.status << socket_t::tc_complete;
    res.response << protocol::standby;
    return res;
  };

  // standby during init status
  while (c -> is_connected() && access_game(c -> game_id) -> thread_com == socket_t::tc_init) {
    if (!c -> check_protocol(protocol::any, handler)) {
      c -> set_disconnect();
    }
  }

  // disconnect if we didn't pass to run status
  if (c -> is_connected() && *(c -> thread_com) != socket_t::tc_run) {
    c -> set_disconnect();
  }
}

// at this point, no other thread will access c -> clients
void handler::dispatch_game(string gid) {
  com *c = access_game(gid);

  // wait for client wfg threads to finish
  for (auto cl : c -> access_clients()) if (cl -> wfg_thread) end_thread(cl -> wfg_thread);

  try {
    game_data g;
    g.settings = c -> settings;
    g.build_players(c -> access_clients());
    g.build();  
    game_handler(*c, g);
  } catch (exception &e) {
    log(e.what());
  } catch (...) {
    log("Unknown object thrown by game handler");
  }

  c -> disconnect();
}

handler::handler() {
  status = socket_t::tc_run;
}

void handler::dispatch_client(client_t *c) {
  local_output("dispatch_client: start");

  query_handler join_handler = [this, c] (int cid, sf::Packet p) -> handler_result {
    handler_result res;
    handler_result res_invalid;
    string gid;
    string name;
    client_game_settings c_settings;
    bool test = p >> gid >> name >> c_settings;
    local_output("join_handler: start");
    if (!test) local_output("join_handler: failed to unpack");

    // guarantee server status does not change until the game has been
    // created
    game_ring.lock();
    test = test && status == socket_t::tc_run && valid_string(gid) && valid_string(name) && c_settings.validate();

    res.status = socket_t::tc_complete;
    res.response << protocol::confirm;

    res_invalid.status = socket_t::tc_stop;
    res_invalid.response << protocol::invalid;

    if (!(test && status == socket_t::tc_run)) {
      local_output("join_handler: failed condition");
      game_ring.unlock();
      return res_invalid;
    }

    com *link = access_game(gid, false);
    bool game_is_new = false;
    
    if (!link) {
      local_output("join_handler: creating new game");
      game_is_new = true;
      link = create_game(gid, c_settings, false);
    }
	  
    link -> lock();

    // test if joinable
    if (link -> can_join()) {
      local_output("join_handler: can join!");
      link -> add_client(c);
      c -> name = name;
      c -> game_id = gid;
      res.response << c -> id;
    } else {
      local_output("join_handler: can not join!");
      res = res_invalid;
    }
    
    link -> unlock();
    game_ring.unlock();

    local_output("join_handler: complete");
    return res;
  };

  // temporarily use server status as thread com
  c -> thread_com = &status;
  if (c -> check_protocol(protocol::connect, join_handler)) {
    local_output("dispatch_client: starting join thread");
    c -> wfg_thread = new thread([this,c] () {wfg(c);});
  } else {
    local_output("dispatch_client: failed protocol, deallocating");
    delete c;
  }
}

void handler::run() {    
  sf::TcpListener listener;

  cout << "binding listener ...";
  // bind the listener to a port
  if (listener.listen(53000) != sf::Socket::Done) {
    cout << "failed." << endl;
    return;
  }
  listener.setBlocking(false);

  cout << "done." << endl;

  // start worker threads
  thread t_game_dispatch([this] () {game_dispatcher();});
  
  // accept connections
  while (status == socket_t::tc_run) {
    client_t *test = new client_t();
    sf::Socket::Status code;
      
    while ((code = listener.accept(*test)) == sf::Socket::Partial) {}
    if (code == sf::Socket::Done) {
      dispatch_client(test);
    } else {
      test -> disconnect();
      delete test;
    }

    sf::sleep(sf::milliseconds(500));
  }

  cout << "server: handler: waiting for threads." << endl;
  t_game_dispatch.join();
  listener.close();
  cout << "server: handler: completed!" << endl;
}

void handler::handle_sigint() {
  cout << "server: caught signal SIGINT, stopping..." << endl;

  game_ring.lock();
  status = socket_t::tc_stop;
  for (auto &c : games) c.second.thread_com = socket_t::tc_stop;
  game_ring.unlock();
    
  while (games.size()) {
    local_output("Waiting for clients to terminate...");
    sf::sleep(sf::milliseconds(100));
  }

  cout << "server: all games terminated! Exiting." << endl;
  status = socket_t::tc_complete;
}

void handler::game_dispatcher() {
  list<string> rbuf;
  local_output("game_dispatcher: start");
  
  while (status != socket_t::tc_complete) {
    rbuf.clear();

    game_ring.lock();
    
    // check games to launch
    for (auto &c : games) {
      com &game = c.second;
      string gid = c.first;
      list<client_t*> clients = game.access_clients();

      // only cleanup clients for games in init status
      if (game.thread_com == socket_t::tc_init) {
	for (auto c : clients) {
	  if (c -> wfg_thread && !c -> is_connected()) {
	    local_output("game_dispatcher: removing disconnected client " + to_string(c -> id) + " from game " + gid);
	    end_thread(c -> wfg_thread);
	    game.clients.erase(c -> id);
	  }
	}
      }
      
      if (game.ready_to_launch()) {
	local_output("game_dispatcher: setting game " + gid + " to run status!");
	game.thread_com = socket_t::tc_run;
	game.active_thread = new thread([this, gid] () {dispatch_game(gid);});
      }

      if (game.clients.empty()) {
	local_output("game_dispatcher: game " + gid + " has no clients, listing for removal.");
	rbuf.push_back(gid);
      }
    }
    game_ring.unlock();

    // remove games
    for (auto v : rbuf) {
      local_output("game_dispatcher: register game " + v + " for termination.");
      com *g = access_game(v);
      if (g -> active_thread) end_thread(g -> active_thread);
      games.erase(v);
      local_output("game_dispatcher: completed terminating game " + v);
    }
      
    sf::sleep(sf::milliseconds(100));
  }
}
