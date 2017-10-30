#include <iostream>

#include "selector.h"
#include "com_client.h"
#include "protocol.h"
#include "serialization.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace st3::client;

void st3::client::load_frames(socket_t *socket, vector<data_frame> &g, int &idx, int &com_in, int &com_out) {
  sf::Packet pq;
  int sub_com = socket_t::tc_run;

  for (idx = 0; idx < g.size() && com_in == socket_t::tc_run; idx++) {
    pq.clear();
    pq << protocol::frame << idx;

    query(socket, pq, com_in, sub_com);

    if (sub_com != socket_t::tc_complete) {
      com_out = sub_com;
      break;
    }

    deserialize(g[idx], socket -> data, socket -> id);
  }

  if (!(com_in | com_out == socket_t::tc_run)) return;

  // indicate done
  int i = -1;
  pq.clear();
  pq << protocol::frame << i;
  query(socket, pq, com_in, com_out);
}

void st3::client::query(socket_t *socket, sf::Packet &pq, int &com_in, int &com_out) {
  protocol_t message;

  socket -> thread_com = &com_in;
  if (!socket -> send_packet(pq)) {
    if (com_in != socket_t::tc_run) {
      // communication stopped
      return;
    } else {
      throw network_error("client::query: failed to send packet");
    }
  }
  
  if (!socket -> receive_packet()) {
    if (com_in != socket_t::tc_run) {
      // communication stopped
      return;
    } else {
      throw network_error("client::query: failed to receive packet");
    }
  }

  if (socket -> data >> message){
    if (message == protocol::confirm){
      com_out = socket_t::tc_complete;
    }else if (message == protocol::standby){
      // wait a little while then try again
      sf::sleep(sf::milliseconds(500));
      query(socket, pq, com_in, com_out);
    }else if (message == protocol::invalid){
      throw network_error("query: server says invalid");
    }else if (message == protocol::complete){
      cout << "query: server says complete" << endl;
      com_out = socket_t::tc_game_complete;
    }else if (message == protocol::aborted){
      cout << "query: server says game aborted" << endl;
      com_out = socket_t::tc_stop;
    }else {
      throw network_error("query: unknown response: " + message);
    }
  }else{
    throw network_error("query: failed to unpack message");
  }
}


template<typename T> 
entity_selector::ptr client::deserialize_object(sf::Packet &p, sint id){
  // assure that T is a properly setup entity selector
  static_assert(is_base_of<client::entity_selector, T>::value, "deserialize entity_selector");
  static_assert(is_base_of<game_object, typename T::base_object_t>::value, "selector base object type must inherit game_object");

  sf::Color col;
  typename T::base_object_t s;
  
  if (!(p >> s)) {
    throw network_error("deserialize_object: package empty!");
  }
  
  return T::create(s, col, s.owner == id);
}

void client::deserialize(data_frame &f, sf::Packet &p, sint id){
  sint n;

  if (f.entity.size()) {
    throw classified_error("client::deserialize: data frame contains entities!");
  }
  
  if (!(p >> f.players >> f.settings >> f.remove_entities >> f.terrain >> n)){
    throw network_error("deserialize: package empty!");
  }

  // "polymorphic" deserialization
  for (int i = 0; i < n; i++){
    class_t key;
    entity_selector::ptr obj;
    p >> key;
    if (key == ship::class_id){
      obj = deserialize_object<client::ship_selector>(p, id);
    }else if (key == fleet::class_id){
      obj = deserialize_object<client::fleet_selector>(p, id);
    }else if (key == solar::class_id){
      obj = deserialize_object<client::solar_selector>(p, id);
    }else if (key == waypoint::class_id){
      obj = deserialize_object<client::waypoint_selector>(p, id);
    }else{
      throw network_error("deserialize: key " + key + " not recognized!");
    }

    if (obj -> owner >= 0) {
      obj -> color = sf::Color(f.players[obj -> owner].color);
    } else {
      obj -> color = sf::Color(150,150,150);
    }
    
    f.entity[obj -> id] = obj;
  }
}

