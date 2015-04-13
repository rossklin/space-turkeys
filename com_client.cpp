#include <iostream>

#include "com_client.h"
#include "protocol.h"
#include "game_data.h"
#include "serialization.h"

using namespace std;
using namespace st3;
using namespace st3::client;

void st3::client::load_frames(socket_t socket, vector<game_data> &g, int &loaded){
  sf::Packet pq, pr;
  bool done;

  sint i = 0;
  while (i < g.size()){
    pq.clear();
    pq << protocol::frame << i;
    query(socket, pq, pr, done);
    if (pr >> g[i]){
      i++;
      loaded = i;
    }else{
      sf::sleep(sf::milliseconds(10));
    }
  }
}


// blocking function: send package pq to socket, receive package pr
// from socket, then set the value of done to true (for threading)
void st3::client::query(socket_t socket, 
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

