#include <iostream>
#include <queue>
#include <thread>

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
      }else if (input == protocol::leave){
	cout << "client " << id << " disconnected!" << endl;
	status = sf::Socket::Status::Disconnected;
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

bool server::client_t::is_connected(){
  return status != sf::Socket::Disconnected;
}

void server::client_t::check_protocol(protocol_t q, sf::Packet &p){
  while (!receive_query(q)){
    if (!is_connected()) return;
    sf::sleep(sf::milliseconds(10));
  }

  while (!send(p)){
    if (!is_connected()) return;
    sf::sleep(sf::milliseconds(10));
  }
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
  list<thread> ts;

  if (packets.size() < clients.size()){
    cout << "check_protocol: to few packets!" << endl;
    return;
  }

  for (unsigned int i = 0; i < clients.size(); i++){
    ts.push_back(thread(&com::client_t::check_protocol, &clients[i], query, ref(packets[i])));
  }

  for (auto t : ts) t.join();

  // remove disconnected clients
  vector<client_t*> buf;
  for (auto i : clients){
    if (i -> is_connected()){
      buf.push_back(i);
    }else{
      delete i;
    }
  }

  clients = buf;
}

void st3::server::com::check_protocol(protocol_t query, sf::Packet &packet){
  vector<sf::Packet> v(clients.size(), packet);
  check_protocol(query, v);
}

void distribute_frames_to(vector<game_data> buf, int &available_frames, client_t *c){
  // debuging purpose, only 2 threads allowed!
  static thread::id tid[2];
  static int tidc = 0;

  tid[tidc++] = this_thread::get_id();
  
  int lim_start = 0;
  int lim_end;
  vector<game_data> g(buf.size());
  int last_idx = g.size() - 1;
  int no_frame = -1;
  int idx = no_frame;
  bool verbose = this_thread::get_id() == tid[0];

  cout << "distribute " << g.size() << " frames to " << c -> id << ": start" << endl;
  
  while (idx < last_idx){
    lim_end = available_frames;
    for (int i = lim_start; i < lim_end; i++)
      g[i] = buf[i].limit_to(c -> id);
    lim_start = lim_end;
    
    if (idx == no_frame && c -> receive_query(protocol::frame)){
      if (!(*(c -> data) >> idx)){
	c -> send_invalid();
	idx = -1;
	exit(-1);
      }

      if (verbose) cout << "client " << c -> id << " requests frame " << idx << endl;

      if (idx == no_frame){
	if (verbose) cout << "client " << c -> id << " is done!" << endl;
	// indicates done
	sf::Packet p;
	p << protocol::confirm;
	c -> send(p);
	break;
      }
    }
    
    if (idx >= 0 && idx < lim_end){
      sf::Packet psend;
      psend << protocol::confirm;
      if (!(psend << g[idx])){
	cout << "failed to serialize frame!" << endl;
	exit(-1);
      }

      if (verbose) cout << "sending frame " << idx << " to client " << c -> id << endl;

      if (c -> send(psend)) idx = no_frame;
    }else if (idx > last_idx){
      cout << "client " << c -> id << " required invalid frame " << idx << endl;
      c -> send_invalid();
      idx = -1;
      exit(-1);
    }
    
    sf::sleep(sf::milliseconds(1));
  }

  cout << "distributed " << idx << " of " << (g.size() - 1) << " frames to " << c -> id << ": end" << endl;
}

void st3::server::com::distribute_frames(vector<game_data> &g, int &frame_count){
  list<thread*> ts;

  cout << "com::distribute_frames: " << clients.size() << " clients:" << endl;
  for (unsigned int i = 0; i < clients.size(); i++){
    thread* t = new thread(distribute_frames_to, ref(g), ref(frame_count), &clients[i]);
    ts.push_back(t);
    cout << clients[i].name << endl;
  }

  for (auto x : ts) {
    x -> join();
    delete x;
  }

  cout << "com::distribute_frames: end" << endl;
}
