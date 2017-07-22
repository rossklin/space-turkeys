#include <iostream>

#include "selector.h"
#include "com_client.h"
#include "protocol.h"
#include "serialization.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace st3::client;

void st3::client::load_frames(socket_t *socket, vector<data_frame> &g, int &loaded, int &done){
  sf::Packet pq;
  int response = 0;

  sint i = 0;
  while (i < g.size() && !done){
    pq.clear();
    pq << protocol::frame << i;

    query(socket, pq, response);

    if (response != query_accepted){
      done |= response;
      break;
    }

    if (deserialize(g[i], socket -> data, socket -> id)){
      i++;
      loaded = i;
    }else{
      sf::sleep(sf::milliseconds(10));
    }
  }

  if (done) return;

  // indicate done
  i = -1;
  pq.clear();
  pq << protocol::frame << i;
  query(socket, pq, response);
  if (response != query_accepted) done |= response;
}

void st3::client::query(socket_t *socket, 
	   sf::Packet &pq,
	   int &done){
  protocol_t message;

  while (!socket -> send_packet(pq)) {
    if (done) return;
    sf::sleep(sf::milliseconds(10));
  }
  
  while (!socket -> receive_packet()) {
    if (done) return;
    sf::sleep(sf::milliseconds(10));
  }

  if (socket -> data >> message){
    if (message == protocol::confirm){
      done = query_accepted;
    }else if (message == protocol::invalid){
      throw runtime_error("query: server says invalid");
    }else if (message == protocol::complete){
      cout << "query: server says complete" << endl;
      done = query_game_complete;
    }else if (message == protocol::aborted){
      cout << "query: server says game aborted" << endl;
      done = query_aborted;
    }else {
      throw runtime_error("query: unknown response: " + message);
    }
  }else{
    throw runtime_error("query: failed to unpack message");
  }
}


template<typename T> 
entity_selector::ptr client::deserialize_object(sf::Packet &p, sint id){
  // assure that T is a properly setup entity selector
  static_assert(is_base_of<client::entity_selector, T>::value, "deserialize entity_selector");
  static_assert(is_base_of<game_object, typename T::base_object_t>::value, "selector base object type must inherit game_object");

  sf::Color col;
  typename T::base_object_t s;
  if (!(p >> s)) throw runtime_error("deserialize_object: package empty!");
  return T::create(s, col, s.owner == id);
}

bool client::deserialize(data_frame &f, sf::Packet &p, sint id){
  sint n;
  
  if (!(p >> f.players >> f.settings >> f.remove_entities >> n)){
    cout << "deserialize: package empty!" << endl;
    return false;
  }

  if (f.entity.size()) throw runtime_error("client::deserialize: data frame contains entities!");

  // "polymorphic" deserialization
  for (int i = 0; i < n; i++){
    class_t key;
    entity_selector::ptr obj;
    p >> key;
    if (key == ship::class_id){
      if (!(obj = deserialize_object<client::ship_selector>(p, id))) return false;
    }else if (key == fleet::class_id){
      if (!(obj = deserialize_object<client::fleet_selector>(p, id))) return false;
    }else if (key == solar::class_id){
      if (!(obj = deserialize_object<client::solar_selector>(p, id))) return false;
    }else if (key == waypoint::class_id){
      if (!(obj = deserialize_object<client::waypoint_selector>(p, id))) return false;
    }else{
      throw runtime_error("deserialize: key " + key + " not recognized!");
    }

    if (obj -> owner >= 0) {
      obj -> color = sf::Color(f.players[obj -> owner].color);
    } else {
      obj -> color = sf::Color(150,150,150);
    }
    
    f.entity[obj -> id] = obj;
  }

  return true;
}

