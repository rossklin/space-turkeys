#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <thread>
#include <ctime>

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

#include "types.h"
#include "com.h"
#include "game_data.h"
#include "graphics.h"
#include "protocol.h"
#include "serialization.h"

using namespace std;
using namespace st3;

// blocking function: send package pq to socket, receive package pr
// from socket, then set the value of done to true (for threading)
void query(socket_t socket, 
	   sf::Packet &pq,
	   sf::Packet &pr,
	   bool &done){
  protocol_t message;

  cout << "query: starting to send..." << endl;
  while (!socket.send(pq)) sf::sleep(sf::milliseconds(100));
  cout << "query: starting to receive..." << endl;
  while (!socket.receive(pr)) sf::sleep(sf::milliseconds(100));

  if (pr >> message){
    if (message == protocol::confirm){
      done = true;
      cout << "query: confirmed" << endl;
    }else if (message == protocol::invalid){
      cout << "query: server says invalid" << endl;
      exit(-1);
    }else if (message == protocol::standby){
      cout << "query: server says standby" << endl;
      exit(-1);
    }else if (message == protocol::complete){
      cout << "query: server says complete" << endl;
      exit(-1);
    }else {
      cout << "query: unknown response: " << message << endl;
      exit(-1);
    }
  }else{
    cout << "query: failed to unpack message" << endl;
    exit(-1);
  }
}

bool pre_step(socket_t socket, window_t &window, game_data &g){
  bool done = false;
  sf::Packet packet, pq;
  pq << protocol::game_round;

  cout << "pre_step: start" << endl;
  
  // todo: handle response: complete
  thread t(query, 
	   socket, 
	   ref(pq),
	   ref(packet), 
	   ref(done));

  while (!done){
    done |= !window.isOpen();

    sf::Event event;
    while (window.pollEvent(event)){
      if (event.type == sf::Event::Closed) window.close();
    }

    window.clear();
    // display "loading data" and some progress
    window.display();

    cout << "pre_step: loading data..." << endl;
    sf::sleep(sf::milliseconds(100));
  }

  cout << "pre_step: waiting for com thread to finish..." << endl;
  t.join();

  if (!(packet >> g)){
    cout << "pre_step: failed to deserialize game_data" << endl;
    exit(-1);
  }

  return true;
}

void choice_step(socket_t socket, window_t &window, game_data g){
  bool done = false;
  choice c;
  sf::Packet pq, pr;
  int count = 0;

  // CREATE THE CHOICE (USER INTERFACE)

  cout << "choice step: start" << endl;

  while (count++ < 100){
    sf::Event event;
    while (window.pollEvent(event)){
      if (event.type == sf::Event::Closed) exit(-1);

      // update choice with event and done
    }

    window.clear();
    draw_universe(window, g, c);
    window.display();

    sf::sleep(sf::milliseconds(100));
  }

  // SEND THE CHOICE TO SERVER
  
  done = false;
  pq << protocol::choice;
  pq << c;

  thread t(query, 
	   socket, 
	   ref(pq),
	   ref(pr), 
	   ref(done));

  cout << "choice step: sending" << endl;

  while (!done){
    done |= !window.isOpen();

    sf::Event event;
    while (window.pollEvent(event)){
      if (event.type == sf::Event::Closed) window.close();
    }

    window.clear();
    // display: sending choice...
    window.display();

    sf::sleep(sf::milliseconds(100));
  }

  cout << "choice step: waiting for query thread" << endl;
  t.join();
  cout << "choice step: complete" << endl;
}

void load_frames(socket_t socket, vector<game_data> &g, int &loaded){
  sf::Packet pq, pr;
  bool done;

  // shifted indexing since g[0] is already loaded, so frame 0 goes in g[1]
  sint i = 0;
  while (i < g.size() - 1){
    pq.clear();
    pq << protocol::frame << i;
    query(socket, pq, pr, done);
    if (pr >> g[i + 1]){
      i++;
      loaded = i + 1;
    }else{
      sf::sleep(sf::milliseconds(100));
    }
  }
}

void simulation_step(socket_t socket, window_t &window, game_data g0){
  bool done = false;
  bool playing = true;
  int idx = 0;
  int loaded = 1;
  vector<game_data> g(g0.settings.frames_per_round + 1);
  g[0] = g0;

  thread t(load_frames, socket, ref(g), ref(loaded));

  while (!done){
    done |= !window.isOpen();

    cout << "simulation loop: frame " << idx << " of " << loaded << " loaded frames, play = " << playing << endl;

    sf::Event event;
    while (window.pollEvent(event)){
      if (event.type == sf::Event::Closed) window.close();
    }

    window.clear();
    draw_universe(window, g[idx]);
    window.display();

    if (playing && idx < loaded - 1){
      idx++;
    }

    sf::sleep(sf::milliseconds(100));    
  }

  t.join();
}

void game_handler(socket_t socket, window_t &window){
  game_data g;

  while (pre_step(socket, window, g)){
    choice_step(socket, window, g);
    simulation_step(socket, window, g);
  }
}

int main(){
  sf::TcpSocket socket;
  int width = 800;
  int height = 600;

  // connect
  cout << "connecting...";
  if (socket.connect("127.0.0.1", 53000) != sf::Socket::Done){
    cout << "client: connection failed." << endl;
    return -1;
  }
  socket.setBlocking(false);
  cout << "done." << endl;

  // create graphics
  window_t window(sf::VideoMode(width, height), "SFML Turkeys!");
  socket_t s(&socket);
  game_handler(s, window);

  socket.disconnect();

  return 0;
}
