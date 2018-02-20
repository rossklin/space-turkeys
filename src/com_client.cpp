#include <iostream>

#include "selector.h"
#include "com_client.h"
#include "protocol.h"
#include "serialization.h"
#include "utility.h"
#include "game_object.h"

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

// unpack entity package and generate corresponding selector objects in data frame
void client::deserialize(data_frame &f, sf::Packet &p, sint id){
  entity_package ep;

  if (f.entity.size()) {
    throw classified_error("client::deserialize: data frame contains entities!");
  }
  
  if (!(p >> ep)){
    throw network_error("deserialize: package empty!");
  }

  f.settings = ep.settings;
  f.players = ep.players;
  f.remove_entities = ep.remove_entities;
  f.terrain = ep.terrain;

  for (auto buf : ep.entity) {
    game_object::ptr x = buf.second;
    sf::Color col;

    if (x -> owner >= 0) {
      col = sf::Color(f.players[x -> owner].color);
    } else {
      col = sf::Color(150,150,150);
    }
    
    entity_selector::ptr obj;
    if (x -> isa(ship::class_id)){
      obj = client::ship_selector::create(*utility::guaranteed_cast<ship>(x), col, x -> owner == id);
    }else if (x -> isa(fleet::class_id)){
      obj = client::fleet_selector::create(*utility::guaranteed_cast<fleet>(x), col, x -> owner == id);
    }else if (x -> isa(solar::class_id)){
      obj = client::solar_selector::create(*utility::guaranteed_cast<solar>(x), col, x -> owner == id);
    }else if (x -> isa(waypoint::class_id)){
      obj = client::waypoint_selector::create(*utility::guaranteed_cast<waypoint>(x), col, x -> owner == id);
    }else{
      throw network_error("com_client::deserialize: Failed sanity check: class not recognized!");
    }
    
    f.entity[obj -> id] = obj;
  }

  // deallocate temporary entity data
  ep.clear_entities();
}

