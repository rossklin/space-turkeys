#include <iostream>
#include <queue>
#include <thread>

#include "com_server.h"
#include "protocol.h"
#include "serialization.h"

using namespace std;
using namespace st3;
using namespace st3::server;

void st3::server::client_t::send_invalid(){
  sf::Packet p;
  p << protocol::invalid;
  if(!send_packet(p)){
    cout << "client_t::failed to send: invalid" << endl;
  }
}

bool st3::server::client_t::receive_query(protocol_t query){
  protocol_t input;

  if (receive_packet()){
    if (data >> input){
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

  while (!send_packet(p)){
    if (!is_connected()) return;
    sf::sleep(sf::milliseconds(10));
  }
}

void server::com::disconnect(){
  for (auto c : clients){
    c.second -> disconnect();
    delete c.second;
  }
  clients.clear();
}

bool server::com::connect(int num_clients){
  sf::TcpListener listener;

  disconnect();

  cout << "binding listener ...";
  // bind the listener to a port
  if (listener.listen(53000) != sf::Socket::Done) {
    cout << "failed." << endl;
    return false;
  }

  cout << "done." << endl;
  
  // accept a new connection
  for (int i = 0; i < num_clients; i++){
    cout << "listening...";
    clients[i] = new client_t();
    clients[i] -> id = i;
    if (listener.accept(*clients[i]) != sf::Socket::Done) {
      cout << "error." << endl;
      disconnect();
      return false;
    }
    clients[i] -> setBlocking(false);
    cout << "client connected!" << endl;
  }

  listener.close();

  cout << "starting with " << clients.size() << " clients" << endl;
  return true;
}

bool server::com::introduce(){
  // formalities
  hm_t<sint, sf::Packet> packets;

  for (auto c : clients)
    packets[c.first] << protocol::confirm << c.second -> id;

  check_protocol(protocol::connect, packets);

  for (auto c : clients){
    if (!(c.second -> data >> c.second -> name)){
      cout << "client " << c.first << " failed to provide name." << endl;
      exit(-1);
    }
  }
}

void st3::server::com::check_protocol(protocol_t query, hm_t<sint, sf::Packet> &packets){
  list<thread> ts;

  if (packets.size() < clients.size()){
    cout << "check_protocol: to few packets!" << endl;
    return;
  }

  for (auto c : clients){
    ts.push_back(thread(&server::client_t::check_protocol, c.second, query, ref(packets[c.first])));
  }

  for (auto &t : ts) t.join();

  // remove disconnected clients
  for (auto i = clients.begin(); i != clients.end(); i++)
    if (!i -> second -> is_connected()) clients.erase((i++) -> first);
}

void st3::server::com::check_protocol(protocol_t query, sf::Packet &packet){
  hm_t<sint, sf::Packet> v;
  for (auto c : clients) v[c.first] = packet;
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
      if (!(c -> data >> idx)){
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
	c -> send_packet(p);
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

      if (c -> send_packet(psend)) idx = no_frame;
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
  list<thread> ts;

  cout << "com::distribute_frames: " << clients.size() << " clients:" << endl;
  for (auto c : clients){
    ts.push_back(thread(distribute_frames_to, ref(g), ref(frame_count), c.second));
    cout << c.second -> name << endl;
  }

  for (auto &x : ts) x.join();

  cout << "com::distribute_frames: end" << endl;
}
