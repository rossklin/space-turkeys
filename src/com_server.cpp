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

// ****************************************
// Class client_t
// ****************************************

handler_result client_t::receive_query(query_handler f){
  protocol_t *input;
  handler_result res;
  res.status = socket_t::tc_failed;

  if (receive_packet()) {
    if (input = (protocol_t*)data.getData()) {
      if (*input == protocol::leave) {
	cout << "client " << id << " disconnected!" << endl;
	set_disconnect();
      } else {
	res = f(id, data);
      }
    } else {
      throw network_error("client_t::receive_query: failed to unpack!");
    }
  }

  return res;
}

void client_t::set_disconnect() {
  disconnect();
  status = sf::Socket::Status::Disconnected;
}

bool client_t::is_connected(){
  return status != sf::Socket::Disconnected;
}

bool client_t::check_protocol(query_handler f) {
  bool running = true;
  sf::Packet p_aborted;
  p_aborted << protocol::aborted;

  try {
    while (running && *thread_com == socket_t::tc_run) {
      handler_result res = receive_query(f);

      if (res.status == socket_t::tc_failed && is_connected()) {
	send_packet(p_aborted);
	set_disconnect();
      }

      if (!is_connected()) break;

      send_packet(res.response);

      if (res.status == socket_t::tc_stop) {
	set_disconnect();
	break;
      }

      running = res.status == socket_t::tc_run;
    }
  } catch (network_error e) {
    // network error
    set_disconnect();
  }

  return is_connected();
}

// ****************************************
// Class com
// ****************************************

com::com() {
  idc = 0;
  thread_com = socket_t::tc_init;
}

query_handler com::basic_query_handler(protocol_t expected_query, protocol_t r) {
  sf::Packet response;
  response << r;
  return basic_query_handler(expected_query, response);
}

query_handler com::basic_query_handler(protocol_t expected_query, sf::Packet response) {
  return [expected_query, response] (int cid, sf::Packet query) -> handler_result {
    handler_result res;
    protocol_t input;
    res.status = socket_t::tc_failed;

    if (query >> input) {
      if (input == expected_query || expected_query == protocol::any) {
	res.response = response;
	res.status = socket_t::tc_complete;
      } else {
	res.response << protocol::invalid;
	res.status = socket_t::tc_stop;
      }
    }

    return res;
  };
}

query_handler com::basic_query_handler(protocol_t expected_query, hm_t<int, sf::Packet> responses) {
  return [expected_query, responses] (int cid, sf::Packet query) -> handler_result {
    handler_result res;
    protocol_t input;
    res.status = socket_t::tc_failed;

    if (query >> input) {
      if (input == expected_query || expected_query == protocol::any) {
	res.response = responses.at(cid);
	res.status = socket_t::tc_complete;
      } else {
	res.response << protocol::invalid;
	res.status = socket_t::tc_stop;
      }
    }

    return res;
  };
}

void com::add_client(client_t *c) {
  c -> thread_com = &thread_com;
  clients[c -> id] = c;
}

void com::disconnect(){
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
      clients.begin() -> second -> check_protocol(com::basic_query_handler(protocol::any, packet));
    }
    return false;
  }

  return true;
}

bool com::check_protocol(query_handler f){
  list<thread*> ts;

  for (auto c : clients){
    ts.push_back(new thread(&client_t::check_protocol, c.second, f));
  }

  for (auto &t : ts){
    t -> join();
    delete t;
  }
  
  return cleanup_clients();
}
