#include <iostream>
#include <queue>
#include <thread>

#include "com_server.h"
#include "protocol.h"
#include "serialization.h"
#include "game_data.h"

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
      if (input == query || query == protocol::any){
	return true;
      }else if (input == protocol::leave){
	cout << "client " << id << " disconnected!" << endl;
	disconnect();
	status = sf::Socket::Status::Disconnected;
      }else{
	throw runtime_error("client_t::receive_query: input " + to_string(input) + " does not match query " + to_string(query));
      }
    }else{
      throw runtime_error("client_t::receive_query: failed to unpack!");
    }
  }
  return false;
}

bool server::client_t::is_connected(){
  return status != sf::Socket::Disconnected;
}

bool server::client_t::check_protocol(protocol_t q, sf::Packet &p){
  cout << "client " << name << ": checking protocol: " << q << endl;
  while (!receive_query(q)){
    if (!is_connected()) return false;
    sf::sleep(sf::milliseconds(10));
  }
  cout << "client " << name << ": received protocol: " << q << endl;

  while (!send_packet(p)){
    if (!is_connected()) return false;
    sf::sleep(sf::milliseconds(10));
  }

  cout << "client " << name << ": sent response to protocol" << endl;
  return true;
}

server::com::com() {
  idc = 0;
  running = false;
  complete = false;
}

void server::com::disconnect(){
  for (auto c : clients){
    c.second -> disconnect();
    delete c.second;
  }
  clients.clear();
}

bool com::cleanup_clients(){

  // remove disconnected clients
  auto buf = clients;
  for (auto i = buf.begin(); i != buf.end(); i++){
    if (!i -> second -> is_connected()) {
      delete i -> second;
      clients.erase(i -> first);
      cout << "removed disconnected client: " << i -> first << endl;
    }
  }

  if (clients.size() < 2){
    sf::Packet packet;
    cout << "Less than two clients remaining!" << endl;
    if (!clients.empty()){
      packet << protocol::aborted << string("You are the last player in the game!");
      clients.begin() -> second -> check_protocol(protocol::any, packet);
    }
    return false;
  }

  return true;
}

bool st3::server::com::check_protocol(protocol_t query, hm_t<sint, sf::Packet> &packets){
  list<thread*> ts;

  if (packets.size() < clients.size()){
    cout << "check_protocol: to few packets!" << endl;
    return false;
  }

  for (auto c : clients){
    ts.push_back(new thread(&server::client_t::check_protocol, c.second, query, ref(packets[c.first])));
  }

  for (auto &t : ts){
    t -> join();
    delete t;
  }
  
  return cleanup_clients();
}

bool st3::server::com::check_protocol(protocol_t query, sf::Packet &packet){
  hm_t<sint, sf::Packet> v;
  for (auto c : clients) v[c.first] = packet;
  return check_protocol(query, v);
}

void distribute_frames_to(vector<entity_package> &buf, int &available_frames, client_t *c){
  // debuging purpose, only 2 threads allowed!
  static thread::id tid;
  static bool tidc = false;

  if (!tidc){
    tid = this_thread::get_id();
    tidc = true;
  }
  
  int lim_start = 0;
  int lim_end;
  vector<entity_package> g(buf.size());
  int last_idx = g.size() - 1;
  int no_frame = -1;
  int idx = no_frame;
  bool verbose = true;
  sf::Packet p;
  // bool verbose = this_thread::get_id() == tid;

  cout << "distribute " << g.size() << " frames to " << c -> id << endl;
  
  while (true){
    p.clear();
    
    lim_end = available_frames;
    for (int i = lim_start; i < lim_end; i++) {
      g[i] = buf[i];
      g[i].limit_to(c -> id);
    }
    
    lim_start = lim_end;
    
    if (idx == no_frame && c -> receive_query(protocol::frame)){
      if (!(c -> data >> idx)){
	c -> send_invalid();
	throw runtime_error("distribute_frames_to: failed to load index!");
      }

      if (verbose) cout << "client " << c -> id << " requests frame " << idx << endl;

      if (idx == no_frame){
	if (verbose) cout << "client " << c -> id << " is done!" << endl;
	p << protocol::confirm;
	c -> send_packet(p);
	break;
      }
    }

    // check disconnect
    if (!c -> is_connected()) {
      throw runtime_error("client " + to_string(c -> id) + " disconnected during distribute!");
    }
    
    if (idx >= 0 && idx < lim_end){
      p << protocol::confirm;
      if (!(p << g[idx])) throw runtime_error("failed to serialize frame!");
      if (verbose) cout << "sending frame " << idx << " to client " << c -> id << endl;
      if (c -> send_packet(p)) {
	idx = no_frame;
      }else{
	if (verbose) cout << "Failed to send packet with frame " << idx << " to client " << c -> id << " (retrying)" << endl;
      }
    }else if (idx > last_idx){
      c -> send_invalid();
      throw runtime_error( "client " + to_string(c -> id) + " required invalid frame!");
    }
    
    sf::sleep(sf::milliseconds(1));
  }

  cout << "distributed " << idx << " of " << (g.size() - 1) << " frames to " << c -> id << ": end" << endl;
}

void try_distribute_frames_to(vector<entity_package> &buf, int &available_frames, client_t *c){
  try {
    distribute_frames_to(buf, available_frames, c);
  }catch (exception &e){
    cout << "try distribute: exception: " << e.what() << endl;
  }
  
  return;
}

void st3::server::com::distribute_frames(vector<entity_package> &g, int &frame_count){
  list<thread*> ts;

  cout << "com::distribute_frames: " << clients.size() << " clients:" << endl;
  for (auto c : clients){
    ts.push_back(new thread(try_distribute_frames_to, ref(g), ref(frame_count), c.second));
  }

  for (auto &x : ts) {
    x -> join();
    delete x;
  }

  cleanup_clients();
  cout << "com::distribute_frames: end" << endl;
}
