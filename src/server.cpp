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

using namespace std;
using namespace st3;
using namespace st3::server;

// get number of clients in game by id
int gid_num_clients(string gid) {
  return 2;
}

void dispatch_game(com &c) {
  game_data g;
  c.running = true;
  g.build_players(c.clients);
  g.build();
  game_handler(c, g);
  c.disconnect();
  c.complete = true;
}

struct handler {
  sf::TcpListener listener;
  hm_t<string, server::com> clients;
  hm_t<string, thread*> threads;

  void dispatch_client(client_t *c) {
    c -> setBlocking(true);
    cout << "client connected!" << endl;

    auto protocol_to_string = [c] (protocol_t x, sf::Packet &p) -> string {
      string result;
      if (!c -> check_protocol(x, p)) {
	throw runtime_error("dispatch_client: failed to require protocol " + to_string(x));
      }

      if (!(c -> data >> result)){
	throw runtime_error("client failed to provide data for " + to_string(x));
      }

      return result;
    };
  
    sf::Packet p;

    // assign client to game
    p << protocol::confirm;
    string game_id = protocol_to_string(protocol::connect, p);
    server::com &link = clients[game_id];

    if (link.running) {
      cout << "client tried to join running game" << endl;
      c -> disconnect();
      delete c;
      return;
    }
  
    c -> id = link.idc++;
    link.clients[c -> id] = c;

    // send id and get name
    p.clear();
    p << protocol::confirm << c -> id;
    c -> name = protocol_to_string(protocol::id, p);

    // if game is full, start it
    if (link.clients.size() == gid_num_clients(game_id)) {
      cout << "starting game " << game_id << endl;
      threads[game_id] = new thread(dispatch_game, ref(link));
    }
  }

  void run() {    
    cout << "binding listener ...";
    // bind the listener to a port
    if (listener.listen(53000) != sf::Socket::Done) {
      cout << "failed." << endl;
      return;
    }

    cout << "done." << endl;

    // start cleanup worker
    thread t([this] () {cleanup_clients();});
  
    // accept connections
    while (true) {
      cout << "listening...";
      client_t *test = new client_t();
      if (listener.accept(*test) != sf::Socket::Done) {
	cout << "error." << endl;
	test -> disconnect();
	delete test;
      }
      dispatch_client(test);
    }
  }

  void cleanup_clients() {
    list<string> rbuf;
    while (true) {
      rbuf.clear();
      for (auto &x : clients) {
	if (x.second.complete) {
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
  sf::TcpListener listener;
  int num_clients = argc == 2 ? atoi(argv[1]) : 2;
  handler h;

  game_data::confirm_data();
  utility::init();
  h.run();

  return 0;
}
