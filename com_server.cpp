#include <iostream>
#include <queue>

#include "com_server.h"
#include "protocol.h"
#include "serialization.h"

using namespace std;
using namespace st3;
using namespace st3::server;

typedef pair<client_t*, int> cfi;

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
    if (*data >> input){
      if (input == query){
	return true;
      }else{
	cout << "client_t::receive_query: input " << input << " does not match query " << query << endl;
	exit(-1);
      }
    }else{
      cout << "client_t::receive_query: failed to unpack!" << endl;
      exit(-1);
    }
  }
  return false;
}

st3::server::com::com(vector<sf::TcpSocket*> c){
  cout << "com: allocating " << c.size() << " clients" << endl;
  clients.resize(c.size());
  for (sint i = 0; i < clients.size(); i++){
    clients[i].socket = c[i];
    clients[i].id = i;
    clients[i].allocate_packet();
  }
}

void st3::server::com::deallocate(){
  cout << "com::~com: dealocating " << clients.size() << " clients" << endl;
  for (auto x : clients) {
    x.deallocate_packet();
  }
}

void st3::server::com::check_protocol(protocol_t query, vector<sf::Packet> &packets){
  bool run = true;
  queue<cfi> q_in, q_out;

  if (packets.size() < clients.size()){
    cout << "check_protocol: to few packets!" << endl;
    return;
  }

  for (unsigned int i = 0; i < clients.size(); i++){
    q_in.push(cfi(&clients[i], i));
  }

  cout << "com::check_protocol(multi): start" << endl;
  while (q_in.size() || q_out.size()){
    bool passage = false;

    // wait for these clients to send query
    if (q_in.size()){
      cfi c = q_in.front();
      q_in.pop();

      if (c.first -> receive_query(query)){
	q_out.push(c);
      }else{
	q_in.push(c);
      }
    }

    // reply to these clients with package
    if (q_out.size()){
      cfi c = q_out.front();
      q_out.pop();
	
      if (!c.first -> send(packets[c.second])){
	cout << "com: failed to send packet" << endl;
	q_out.push(c);
      }
    }

    sf::sleep(sf::milliseconds(100));
  }
  cout << "com::check_protocol(multi): end" << endl;
}

void st3::server::com::check_protocol(protocol_t query, sf::Packet &packet){
  bool run = true;
  queue<client_t*> q_in, q_out;

  for (unsigned int i = 0; i < clients.size(); i++){
    q_in.push(&clients[i]);
  }

  cout << "com::check_protocol: start" << endl;
  while (q_in.size() || q_out.size()){
    bool passage = false;

    // wait for these clients to send query
    if (q_in.size()){
      client_t *c = q_in.front();
      q_in.pop();

      if (c -> receive_query(query)){
	q_out.push(c);
      }else{
	q_in.push(c);
      }
    }

    // reply to these clients with package
    if (q_out.size()){
      client_t *c = q_out.front();
      q_out.pop();
	
      if (!c -> send(packet)){
	cout << "com: failed to send packet" << endl;
	q_out.push(c);
      }
    }

    sf::sleep(sf::milliseconds(100));
  }
  cout << "com::check_protocol: end" << endl;
}

void st3::server::com::distribute_frames(vector<sf::Packet> &g, int &frame_count){
  queue<client_t*> q_in;
  queue<cfi> q_out;

  for (unsigned int i = 0; i < clients.size(); i++){
    q_in.push(&clients[i]);
  }

  cout << "com::distribute_frames: start" << endl;
  while (q_in.size() || q_out.size()){
    if (q_in.size()){
      client_t* c = q_in.front();
      q_in.pop();

      if (c -> receive_query(protocol::frame)){
	int idx;
	if (*(c -> data) >> idx){
	  q_out.push(cfi(c, idx));
	}else{
	  cout << "distribute_frames: failed to unpack idx" << endl;
	  c -> send_invalid();
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
	if (c.first -> send(g[c.second])){
	  cout << "com: sent frame " << c.second << " to client " << c.first -> name << endl;
	  if (c.second < frame_count - 1){
	    q_in.push(c.first);
	  }
	}else{
	  cout << "failed to send frame, retrying" << endl;
	  q_out.push(c);
	}
      }else if (c.second >= 0 && c.second < g.size()){
	cout << "distribute_frames: index " << c.second << " not created, waiting" << endl;
	q_out.push(c);
      }else{
	cout << "invalid index: " << c.second << " (cmp " << g.size() << ")" <<endl;
	c.first -> send_invalid();
	q_in.push(c.first);
      }
    }

    sf::sleep(sf::milliseconds(1));
  }
  cout << "com::distribute_frames: end" << endl;
}
