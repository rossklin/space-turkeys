#include <iostream>
#include <queue>

#include "com.h"
#include "protocol.h"

using namespace std;
using namespace st3;
using namespace st3::server;

typedef pair<client_t, int> cfi;
st3::socket_t::socket_t(){
  socket = 0;
  id = -1;
}

st3::socket_t::socket_t(sf::TcpSocket *s){
  socket = s;
  id = -1;
}

bool st3::socket_t::send(sf::Packet &packet){
  switch(status = socket -> send(packet)){
  case sf::Socket::Disconnected:
    cout << "socket_t::send: disconnected." << endl;
    exit(-1);
  case sf::Socket::Error:
    cout << "socket_t::send: error sending!" << endl;
    exit(-1);
  case sf::Socket::Done:
    cout << "socket_t::send: done" << endl;
    return true;
  case sf::Socket::NotReady:
    cout << "socket_t::send: not ready" << endl;
    return false;
  default:
    cout << "socket_t::send: unknown status: " << status << endl;
    exit(-1);
  }
}

bool st3::socket_t::receive(){
  switch(status = socket -> receive(data)){
  case sf::Socket::Disconnected:
    cout << "socket_t::receive: disconnected." << endl;
    exit(-1);
  case sf::Socket::Error:
    cout << "socket_t::receive: error sending!" << endl;
    exit(-1);
  case sf::Socket::Done:
    cout << "socket_t::receive: done" << endl;
    return true;
  case sf::Socket::NotReady:
    cout << "socket_t::receive: not ready" << endl;
    return false;
  default:
    cout << "socket_t::send: unknown status: " << status << endl;
    exit(-1);
  }
}

bool st3::socket_t::receive(sf::Packet &packet){
  switch(status = socket -> receive(packet)){
  case sf::Socket::Disconnected:
    cout << "socket_t::receive: disconnected." << endl;
    exit(-1);
  case sf::Socket::Error:
    cout << "socket_t::receive: error sending!" << endl;
    exit(-1);
  case sf::Socket::Done:
    cout << "socket_t::receive: done" << endl;
    return true;
  case sf::Socket::NotReady:
    cout << "socket_t::receive: not ready" << endl;
    return false;
  default:
    cout << "socket_t::send: unknown status: " << status << endl;
    exit(-1);
  }
}

void st3::server::client_t::send_invalid(){
  sf::Packet p;
  p << protocol::invalid;
  if(!send(p)){
    cout << "client_t::failed to send: invalid" << endl;
  }
}

bool st3::server::client_t::receive_query(protocol_t query){
  protocol_t input;

  if (receive()){
    if (data >> input){
      if (input == query){
	return true;
      }else{
	cout << "client_t::receive_query: input " << input << " does not match query " << query << endl;
	send_invalid();
      }
    }else{
      cout << "client_t::receive_query: failed to unpack!" << endl;
      send_invalid();
    }
  }
  return false;
}

st3::server::com::com(vector<sf::TcpSocket*> c){
  clients.resize(c.size());
  for (unsigned int i = 0; i < clients.size(); i++){
    clients[i].socket = c[i];
  }
}

void st3::server::com::check_protocol(protocol_t query, sf::Packet &packet){
  bool run = true;
  queue<client_t> q_in, q_out;

  for (auto x : clients){
    q_in.push(x);
  }

  cout << "com::check_protocol: start" << endl;
  while (q_in.size() || q_out.size()){
    bool passage = false;

    // wait for these clients to send query
    if (q_in.size()){
      client_t c = q_in.front();
      q_in.pop();

      if (c.receive_query(query)){
	q_out.push(c);
      }else{
	q_in.push(c);
      }
    }

    // reply to these clients with package
    if (q_out.size()){
      client_t c = q_out.front();
      q_out.pop();
	
      if (!c.send(packet)){
	cout << "com: failed to send packet" << endl;
	q_out.push(c);
      }
    }

    sf::sleep(sf::milliseconds(100));
  }
  cout << "com::check_protocol: end" << endl;
}

void st3::server::com::distribute_frames(vector<sf::Packet> &g, int &frame_count){
  queue<client_t> q_in;
  queue<cfi> q_out;

  for (auto x : clients){
    q_in.push(x);
  }

  cout << "com::distribute_frames: start" << endl;
  while (q_in.size() || q_out.size()){
    if (q_in.size()){
      client_t c = q_in.front();
      q_in.pop();

      if (c.receive_query(protocol::frame)){
	int idx;
	if (c.data >> idx){
	  q_out.push(cfi(c, idx));
	}else{
	  cout << "distribute_frames: failed to unpack idx" << endl;
	  c.send_invalid();
	  q_in.push(c);
	}
      }else{
	q_in.push(c);
      }
    }

    if (q_out.size()){
      cfi c = q_out.front();
      q_out.pop();

      if (c.second >= 0 && c.second < frame_count){
	if (c.first.send(g[c.second])){
	  if (c.second < frame_count - 1){
	    q_in.push(c.first);
	  }
	}else{
	  cout << "failed to send frame, retrying" << endl;
	  q_out.push(c);
	}
      }else{
	cout << "invalid index: " << c.second << " (cmp " << frame_count << ")" <<endl;
	c.first.send_invalid();
	q_in.push(c.first);
      }
    }

    sf::sleep(sf::milliseconds(100));
  }
  cout << "com::distribute_frames: end" << endl;
}
