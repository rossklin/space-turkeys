#include <iostream>

#include "game_handler.h"
#include "game_data.h"
#include "server_handler.h"
#include "serialization.h"
#include "protocol.h"

using namespace std;
using namespace st3;
using namespace server;

bool valid_string(string v) {
  // todo
  return v.length() < 1000;
}

void end_thread(thread *&t) {
  t -> join();
  delete t;
  t = 0;
}

// at this point, no other thread will access c -> clients
void handler::dispatch_game(string gid) {
  com *c = access_game(gid);
  c -> thread_com = socket_t::tc_run;

  // wait for client wfg threads to finish
  for (auto cl : c -> access_clients()) if (cl -> wfg_thread) end_thread(cl -> wfg_thread);

  try {
    game_data g;
    g.settings = c -> settings;
    g.build_players(c -> access_clients());
    g.build();  
    game_handler(*c, g);
  } catch (exception e) {
    cout << "Unhandled exception in game_handler: " << e.what() << endl;
  }

  c -> disconnect();
  c -> thread_com = socket_t::tc_complete;
}

handler::handler() {
  status = socket_t::tc_run;
}

void handler::dispatch_client(client_t *c) {

  cout << "dispatch client:" << endl;

  query_handler join_handler = [this, c] (int cid, sf::Packet p) -> handler_result {
    handler_result res;
    handler_result res_invalid;
    string gid;
    string name;
    client_game_settings c_settings;
    bool test = p >> gid >> name >> c_settings;

    // guarantee server status does not change until the game has been
    // created
    status_ring.lock();
    test = test && status == socket_t::tc_run && valid_string(gid) && valid_string(name) && c_settings.validate();

    res.status = socket_t::tc_complete;
    res.response << protocol::confirm;

    res_invalid.status = socket_t::tc_stop;
    res_invalid.response << protocol::invalid;

    if (!(test && status == socket_t::tc_run)) {
      return res_invalid;
    }

    com *link = access_game(gid);
    bool game_is_new = false;
    
    if (!link) {
      game_is_new = true;
      link = create_game(gid, c_settings);
    }
    status_ring.unlock();
	  
    link -> lock();

    // test if joinable
    if (link -> can_join()) {
      link -> add_client(c);
      c -> name = name;
      c -> game_id = gid;
      res.response << c -> id;
    } else {
      res = res_invalid;
    }
    
    link -> unlock();
    return res;
  };

  // temporarily use server status as thread com
  c -> thread_com = &status;
  if (c -> check_protocol(protocol::connect, join_handler)) {
    c -> wfg_thread = new thread([this,c] () {wfg(c);});
  } else {
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

  status_ring.lock();
  status = socket_t::tc_stop;
  for (auto &c : games) c.second.thread_com = socket_t::tc_stop;
  status_ring.unlock();
    
  while (games.size()) {
    cout << "Waiting for clients to terminate..." << endl;
    sf::sleep(sf::milliseconds(100));
  }
    
  status = socket_t::tc_complete;
}

void handler::game_dispatcher() {
  list<string> rbuf;
  while (status != socket_t::tc_complete) {

    game_ring.lock();
    // check games to launch
    for (auto &c : games) {
      com &game = c.second;
      string gid = c.first;
      if (game.ready_to_launch()) {
	game.active_thread = new thread([this, gid] () {dispatch_game(gid);});
      }
    }
    
    // check games to remove
    rbuf.clear();
    for (auto &x : games) {
      bool is_complete = x.second.thread_com == socket_t::tc_complete;
      bool is_empty = x.second.access_clients().empty();
      bool is_init = x.second.thread_com == socket_t::tc_init;

      if (is_complete || (is_empty && !is_init)) {
	rbuf.push_back(x.first);
      }
    }
    game_ring.unlock();

    // remove games
    for (auto v : rbuf) {
      cout << "register game " << v << " for termination." << endl;
      com *g = access_game(v);

      for (auto c : g -> access_clients()) {
	if (c -> wfg_thread) end_thread(c -> wfg_thread);
      }
      
      if (g -> active_thread) end_thread(g -> active_thread);
	
      games.erase(v);
      cout << "completed terminating game " << v << endl;
    }
      
    sf::sleep(sf::milliseconds(100));
  }
}
