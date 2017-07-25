#include <iostream>
#include <exception>
#include <cstdlib>
#include <thread>

#include "game_handler.h"
#include "com_server.h"
#include "protocol.h"
#include "utility.h"
#include "cost.h"
#include "research.h"
#include "serialization.h"

using namespace std;
using namespace st3;
using namespace st3::server;

void dispatch_game(com &c) {
  try {
    game_data g;
    g.settings = c.settings;
    c.thread_com = socket_t::tc_run;
    g.build_players(c.clients);
    g.build();  
    game_handler(c, g);
  } catch (exception e) {
    cout << "Unhandled exception in game_handler: " << e.what() << endl;
  }

  c.disconnect();
  c.thread_com = socket_t::tc_complete;
}

// todo: on success, set response to p in handler!
string protocol_to_string(client_t *c, protocol_t x, sf::Packet &p) {
  string result;
  
  query_handler handler = [&result] (int cid, sf::Packet p) -> handler_result {
    return handler_switch(p >> result);
  };

  if (!c -> check_protocol(x, handler)) {
    throw runtime_error("dispatch_client: failed to require protocol " + to_string(x));
  }

  return result;
}

struct handler {
  sf::TcpListener listener;
  hm_t<string, server::com> coms;
  hm_t<string, thread*> threads;
  list<int*> temp_tc;
  int status;

  handler() {
    status = socket_t::tc_run;
  }

  void dispatch_client(client_t *c) {
    c -> setBlocking(true);
    cout << "dispatch client:" << endl;

    sf::Packet p;

    query_handler join_handler = [this, c] (int cid, sf::Packet p) -> handler_result {
      string gid;
      bool test = p >> gid;

      auto on_success = [this, c, gid, p] (handler_result &res) {
	auto set_invalid = [&res] () {
	  res.status = socket_t::tc_stop;
	  res.response.clear();
	  res.response << protocol::invalid;
	};

	sf::Packet pbuf = p;
	  
	c -> game_id = gid;
	bool game_is_new = !coms.count(c -> game_id);
	server::com &link = coms[c -> game_id];
	  
	if (link.thread_com != socket_t::tc_init) {
	  cout << "tried to join running game" << endl;
	  set_invalid();
	  return;
	}

	if (game_is_new) {
	  client_game_settings test_settings;
	  if (!(pbuf >> test_settings)) {
	    cout << "didn't provide settings" << endl;
	    set_invalid();
	    return;
	  }

	  if (!test_settings.validate()) {
	    cout << "provided invalid settings" << endl;
	    set_invalid();
	    return;
	  }

	  static_cast<client_game_settings&>(link.settings) = test_settings;
	}
      };
	
      return handler_switch(test, on_success);
    };
    
    // assign client to game
    int *com_buf = new int(socket_t::tc_run);
    temp_tc.push_back(com_buf);
    c -> thread_com = com_buf;
    if (!c -> check_protocol(protocol::connect, join_handler)) {
      delete c;
      c = 0;
    }
    
    temp_tc.remove(com_buf);
    delete com_buf;

    if (!c) return;

    // client c now has a valid game_id
    server::com &link = coms[c -> game_id];
    try {
      c -> id = link.idc++;
      link.add_client(c);
      // send id and get name
      p.clear();
      p << protocol::confirm << c -> id;
      c -> name = protocol_to_string(c, protocol::id, p);
    } catch (exception e) {
      cout << "Client id exchange exception: " << e.what() << endl;
      link.clients.erase(c -> id);
      c -> disconnect();
      delete c;
      return;
    }    

    // if game is full, start it
    if (link.clients.size() == link.settings.num_players) {
      cout << "starting game " << c -> game_id << endl;
      threads[c -> game_id] = new thread(dispatch_game, ref(link));
    }
  }

  void run() {    
    cout << "binding listener ...";
    // bind the listener to a port
    if (listener.listen(53000) != sf::Socket::Done) {
      cout << "failed." << endl;
      return;
    }
    listener.setBlocking(false);

    cout << "done." << endl;

    // start worker threads
    thread t_cleanup([this] () {cleanup_clients();});
    // disabled for debug:
    // thread t_input([this] () {handle_input();});
  
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
    t_cleanup.join();
    // t_input.join();
    listener.close();
    cout << "server: handler: completed!" << endl;
  }

  // cli to terminate server
  void handle_input() {
    string test;

    while (test != "quit") {
      cin >> test;

      if (test == "info") {
	cout << "games: " << endl;
	for (auto &c : coms) {
	  cout << " - " << c.first << ": " << c.second.thread_com << ": " << c.second.clients.size() << endl;
	}
      }
    }
    
    status = socket_t::tc_stop;
    for (auto &c : coms) c.second.thread_com = socket_t::tc_stop;
    for (auto c : temp_tc) *c = socket_t::tc_stop;
    
    while (coms.size() || temp_tc.size()) {
      cout << "Waiting for clients to terminate..." << endl;
      sf::sleep(sf::milliseconds(100));
    }
    
    status = socket_t::tc_complete;
  }

  void cleanup_clients() {
    list<string> rbuf;
    while (status == socket_t::tc_run || status == socket_t::tc_stop) {
      rbuf.clear();
      for (auto &x : coms) {
	bool is_complete = x.second.thread_com == socket_t::tc_complete;
	bool is_empty = x.second.clients.empty();
	bool is_init = x.second.thread_com == socket_t::tc_init;
	
	if (is_complete || (is_empty && !is_init)) {
	  rbuf.push_back(x.first);
	}
      }

      for (auto v : rbuf) {
	cout << "register game " << v << " for termination." << endl;
	threads[v] -> join();
	delete threads[v];
	threads.erase(v);
	coms.erase(v);
	cout << "completed terminating game " << v << endl;
      }
      
      sf::sleep(sf::milliseconds(100));
    }
  }
};


int main(int argc, char **argv){
  handler h;

  game_data::confirm_data();
  utility::init();
  h.run();

  return 0;
}
