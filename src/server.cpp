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

string protocol_to_string(client_t *c, protocol_t x, sf::Packet &p) {
  string result;
  if (!c -> check_protocol(x, p)) {
    throw runtime_error("dispatch_client: failed to require protocol " + to_string(x));
  }

  if (!(c -> data >> result)){
    throw runtime_error("client failed to provide data for " + to_string(x));
  }

  return result;
}

struct handler {
  sf::TcpListener listener;
  hm_t<string, server::com> clients;
  hm_t<string, thread*> threads;
  int status;

  handler() {
    status = socket_t::tc_run;
  }

  void dispatch_client(client_t *c) {
    c -> setBlocking(true);
    cout << "dispatch client:" << endl;

    string game_id;
    sf::Packet p;
    bool game_is_new = false;
    
    try {
      // assign client to game
      p << protocol::confirm;
      game_id = protocol_to_string(c, protocol::connect, p);
      game_is_new = !clients.count(game_id);
      server::com &link = clients[game_id];
      
      if (link.thread_com != socket_t::tc_init) {
	throw runtime_error("client tried to join running game");
      }

      if (game_is_new) {
	if (!(c -> data >> link.settings)) {
	  throw runtime_error("client failed to provide settings.");
	}

	if (!link.settings.validate()) {
	  throw runtime_error("client provided invalid settings.");
	}
      }
    } catch (exception e) {
      cout << "Client connection exception: " << e.what() << endl;
      c -> disconnect();
      delete c;
      return;
    }

    server::com &link = clients[game_id];
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
    try {
      if (link.clients.size() == link.settings.num_players) {
	cout << "starting game " << game_id << endl;
	threads[game_id] = new thread(dispatch_game, ref(link));
      }
    } catch (exception e) {
      cout << "Dispatch game exception: " << e.what() << endl;
      link.disconnect();
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
    thread t_input([this] () {handle_input();});
  
    // accept connections
    while (status == socket_t::tc_run) {
      cout << "listening...";
      client_t *test = new client_t();
      sf::Socket::Status code;
      
      while ((code = listener.accept(*test)) == sf::Socket::Partial) {}
      if (code == sf::Socket::Done) {
	dispatch_client(test);
      } else {
	cout << " no client found." << endl;
	test -> disconnect();
	delete test;
      }

      sf::sleep(sf::milliseconds(500));
    }

    cout << "server: handler: waiting for threads." << endl;
    t_cleanup.join();
    t_input.join();
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
	for (auto &c : clients) {
	  cout << " - " << c.first << ": " << c.second.thread_com << ": " << c.second.clients.size() << endl;
	}
      }
    }
    
    status = socket_t::tc_stop;
    for (auto &c : clients) c.second.thread_com = socket_t::tc_stop;
    while (clients.size()) {
      cout << "Waiting for games to terminate..." << endl;
      sf::sleep(sf::milliseconds(100));
    }
    
    status = socket_t::tc_complete;
  }

  void cleanup_clients() {
    list<string> rbuf;
    while (status == socket_t::tc_run || status == socket_t::tc_stop) {
      rbuf.clear();
      for (auto &x : clients) {
	if (x.second.thread_com == socket_t::tc_complete || x.second.clients.empty()) {
	  rbuf.push_back(x.first);
	}
      }

      for (auto v : rbuf) {
	cout << "register game " << v << " for termination." << endl;
	threads[v] -> join();
	delete threads[v];
	threads.erase(v);
	clients.erase(v);
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
