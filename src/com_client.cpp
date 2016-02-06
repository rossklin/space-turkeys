#include <iostream>

#include "com_client.h"
#include "protocol.h"
#include "game_data.h"
#include "serialization.h"

using namespace std;
using namespace st3;
using namespace st3::client;

void st3::client::load_frames(socket_t *socket, vector<game_data> &g, int &loaded){
  sf::Packet pq;
  int done = query_query;

  sint i = 0;
  while (i < g.size()){
    pq.clear();
    pq << protocol::frame << i;
    query(socket, pq, done);
    if (socket -> data >> g[i]){
      i++;
      loaded = i;
    }else{
      sf::sleep(sf::milliseconds(1));
    }
  }

  // indicate done
  i = -1;
  pq.clear();
  pq << protocol::frame << i;
  query(socket, pq, done);
}

void st3::client::query(socket_t *socket, 
	   sf::Packet &pq,
	   int &done){
  protocol_t message;

  while (!socket -> send_packet(pq)) sf::sleep(sf::milliseconds(10));
  while (!socket -> receive_packet()) sf::sleep(sf::milliseconds(10));

  if (socket -> data >> message){
    if (message == protocol::confirm){
      done = query_accepted;
    }else if (message == protocol::invalid){
      cout << "query: server says invalid" << endl;
      exit(-1);
    }else if (message == protocol::complete){
      cout << "query: server says complete" << endl;
      done = query_game_complete;
    }else {
      cout << "query: unknown response: " << message << endl;
      exit(-1);
    }
  }else{
    cout << "query: failed to unpack message" << endl;
    exit(-1);
  }
}

