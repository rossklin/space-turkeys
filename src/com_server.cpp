#include "com_server.h"

#include <SFML/Network.hpp>
#include <functional>
#include <iostream>
#include <list>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "game_data.h"
#include "protocol.h"
#include "serialization.h"
#include "server_handler.h"
#include "types.h"

using namespace std;
using namespace st3;
using namespace st3::server;

// ****************************************
// Class client_t
// ****************************************

handler_result server::handler_switch(bool test, function<void(handler_result &)> on_success, function<void(handler_result &)> on_fail) {
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

query_response_generator server::static_query_response(handler_result res) {
  return [res](int cid, sf::Packet data) {
    return res;
  };
}

server::server_cl_socket::server_cl_socket() : socket_t() {
  st3_state = tc_init;
  wfg_thread = 0;
  gid = "";
}

// Receive an expected query from the client and generate a response using the provided callback
handler_result server::server_cl_socket::receive_query(protocol_t p, query_response_generator f) {
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

bool server::server_cl_socket::check_com() {
  return main_status == tc_run;
}

void server::server_cl_socket::set_disconnect() {
  server::log("disconnecting client " + to_string(id));
  disconnect();
  status = sf::Socket::Status::Disconnected;
}

bool server::server_cl_socket::is_connected() {
  return status != sf::Socket::Disconnected;
}

// Until an outcome is reached, wait for receiving a query for the specified
// protocol and process it with the given query response generator
bool server::server_cl_socket::check_protocol(protocol_t p, query_response_generator f) {
  bool running = true;
  bool completed = false;
  sf::Packet p_aborted;
  p_aborted << protocol::aborted;

  output("check protocol " + to_string(p) + ": client " + to_string(id) + ": begin.");

  auto task = [&, this]() {
    while (running && main_status == tc_run) {
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
  };

  auto onfail = [&, this]() {
    // on error
    set_disconnect();
    completed = false;
  };

  safely(task, onfail);

  output("check protocol " + to_string(p) + ": client " + to_string(id) + ": " + to_string(is_connected()));

  return completed;
}

// ****************************************
// Class com
// ****************************************

game_setup::game_setup() {
  client_idc = 0;
  status = socket_t::tc_init;
}

void game_setup::add_client(server_cl_socket_ptr c) {
  clients[c->id] = c;
}

// list<server_cl_socket *> client_communicator::access_clients() {
//   list<server_cl_socket *> res;
//   lock();
//   for (auto &c : clients) res.push_back(c.second);
//   unlock();
//   return res;
// }

// void client_communicator::lock() {
//   m_lock.lock();
// }

// void client_communicator::unlock() {
//   m_lock.unlock();
// }

bool game_setup::can_join() {
  return status == socket_t::tc_init && clients.size() < settings.clset.num_players;
}

bool game_setup::ready_to_launch() {
  if (!(status == socket_t::tc_ready_game && clients.size() == settings.clset.num_players)) return false;
  for (auto c : clients) {
    if (c.second->st3_state != socket_t::tc_run) return false;
  }
  return true;
}

void game_setup::disconnect() {
  for (auto c : clients) {
    c.second->disconnect();
    c.second->st3_state = socket_t::tc_complete;  // tell game monitor to remove this client
  }
  status = socket_t::tc_game_complete;
}

bool game_setup::cleanup_clients() {
  // remove disconnected clients
  auto buf = clients;
  for (auto i = buf.begin(); i != buf.end(); i++) {
    if (!i->second->is_connected()) {
      clients.erase(i->first);
      output("removed disconnected client: " + to_string(i->first));
    }
  }

  if (clients.size() < 2) {
    sf::Packet packet;
    output("Less than two clients remaining!");
    if (!clients.empty()) {
      handler_result res;
      res.response << protocol::aborted << string("You are the last player in the game!");
      res.status = socket_t::tc_complete;

      clients.begin()->second->check_protocol(protocol::any, static_query_response(res));
    }
    return false;
  }

  return true;
}

bool game_setup::check_protocol(protocol_t p, query_response_generator f) {
  list<thread *> ts;

  for (auto c : clients) {
    ts.push_back(new thread(&server_cl_socket::check_protocol, c.second, p, f));
  }

  for (auto &t : ts) {
    t->join();
    delete t;
  }

  return cleanup_clients();
}
