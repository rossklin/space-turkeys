#include <iostream>
#include <queue>
#include <thread>

#include "com_server.h"
#include "protocol.h"
#include "serialization.h"
#include "game_data.h"
#include "server_handler.h"

using namespace std;
using namespace st3;
using namespace st3::server;

// ****************************************
// Class client_t
// ****************************************

handler_result server::handler_switch(bool test, function<void(handler_result&)> on_success, function<void(handler_result&)> on_fail) {
  handler_result res;

  if (test) {
    res.status = socket_t::tc_complete;
    res.response << protocol::confirm;
    if (on_success) on_success(res);
  } else {
    res.status = socket_t::tc_stop;
    res.response << protocol::invalid;
    if (on_fail) on_fail(res);
  }

  return res;
}

handler_result client_t::receive_query(protocol_t p, query_handler f){
  protocol_t input;
  handler_result res;
  res.status = socket_t::tc_failed;

  if (receive_packet()) {
    if (data >> input) {
      if (input == protocol::leave) {
	output("client " + to_string(id) + " disconnected!");
	set_disconnect();
      } else if (input == protocol::standby) {
	res.status = socket_t::tc_run;
	res.response << protocol::confirm;
      } else if (input == p || p == protocol::any) {
	res = f(id, data);
      } else {
	throw network_error("client_t::receive_query: unexpected protocol: " + to_string(input));
      }
    } else {
      throw network_error("client_t::receive_query: failed to unpack!");
    }
  }

  return res;
}

void client_t::set_disconnect() {
  server::log("disconnecting client " + to_string(id));
  disconnect();
  status = sf::Socket::Status::Disconnected;
}

bool client_t::is_connected(){
  return status != sf::Socket::Disconnected;
}

bool client_t::check_protocol(protocol_t p, query_handler f) {
  bool running = true;
  bool completed = false;
  sf::Packet p_aborted;
  p_aborted << protocol::aborted;

  output("check protocol " + to_string(p) + ": client " + to_string(id) + ": begin.");

  handler::safely([&,this] () {
      while (running && (*thread_com == socket_t::tc_run || *thread_com == socket_t::tc_init)) {
	handler_result res = receive_query(p, f);

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
	completed = !running;
      }
    }, [&,this] () {
      // on error
      set_disconnect();
      completed = false;
    });

  output("check protocol " + to_string(p) + ": client " + to_string(id) + ": " + to_string(is_connected()));

  return completed;
}

// ****************************************
// Class com
// ****************************************

com::com() {
  gid = "";
  idc = 0;
  thread_com = socket_t::tc_init;
  active_thread = NULL;
}

void com::add_client(client_t *c) {
  c -> thread_com = &thread_com;
  c -> id = idc++;
  clients[c -> id] = c;
}

list<client_t*> com::access_clients() {
  list<client_t*> res;
  lock();
  for (auto &c : clients) res.push_back(c.second);
  unlock();
  return res;
}

void com::lock() {
  m_lock.lock();
}

void com::unlock() {
  m_lock.unlock();
}

bool com::can_join() {
  return thread_com == socket_t::tc_init && clients.size() < settings.num_players;
}

bool com::ready_to_launch() {
  return thread_com == socket_t::tc_init && clients.size() == settings.num_players;
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
      output("removed disconnected client: " + to_string(i -> first));
    }
  }

  if (clients.size() < 2){
    sf::Packet packet;
    output("Less than two clients remaining!");
    if (!clients.empty()){
      query_handler handler = [] (int cid, sf::Packet q) -> handler_result {
	handler_result res;
	res.response << protocol::aborted << string("You are the last player in the game!");
	res.status = socket_t::tc_complete;
	return res;
      };
      
      clients.begin() -> second -> check_protocol(protocol::any, handler);
    }
    return false;
  }

  return true;
}

bool com::check_protocol(protocol_t p, query_handler f){
  list<thread*> ts;

  for (auto c : clients){
    ts.push_back(new thread(&client_t::check_protocol, c.second, p, f));
  }

  for (auto &t : ts){
    t -> join();
    delete t;
  }
  
  return cleanup_clients();
}
