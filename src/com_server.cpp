#include "com_server.h"

#include <iostream>
#include <queue>
#include <thread>

#include "game_data.h"
#include "protocol.h"
#include "serialization.h"
#include "server_handler.h"

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

// Receive an expected query from the client and generate a response using the provided callback
handler_result client_t::receive_query(protocol_t p, query_response_generator f) {
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

bool client_t::check_com() {
  return main_status == tc_run;
}

void client_t::set_disconnect() {
  server::log("disconnecting client " + to_string(id));
  disconnect();
  status = sf::Socket::Status::Disconnected;
}

bool client_t::is_connected() {
  return status != sf::Socket::Disconnected;
}

// Until an outcome is reached, wait for receiving a query for the specified
// protocol and process it with the given query response generator
bool client_t::check_protocol(protocol_t p, query_response_generator f) {
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

  handler::safely(task, onfail);

  output("check protocol " + to_string(p) + ": client " + to_string(id) + ": " + to_string(is_connected()));

  return completed;
}

// ****************************************
// Class com
// ****************************************

client_communicator::client_communicator() {
  gid = "";
  idc = 0;
  status = socket_t::tc_init;
  active_thread = NULL;
}

void client_communicator::add_client(client_t *c) {
  c->id = idc++;
  clients[c->id] = c;
}

list<client_t *> client_communicator::access_clients() {
  list<client_t *> res;
  lock();
  for (auto &c : clients) res.push_back(c.second);
  unlock();
  return res;
}

void client_communicator::lock() {
  m_lock.lock();
}

void client_communicator::unlock() {
  m_lock.unlock();
}

bool client_communicator::can_join() {
  return status == socket_t::tc_init && clients.size() < settings.num_players;
}

bool client_communicator::ready_to_launch() {
  return status == socket_t::tc_init && clients.size() == settings.num_players;
}

void client_communicator::disconnect() {
  for (auto c : clients) {
    c.second->disconnect();
    delete c.second;
  }
  clients.clear();
  status = socket_t::tc_stop;
}

bool client_communicator::cleanup_clients() {
  // remove disconnected clients
  auto buf = clients;
  for (auto i = buf.begin(); i != buf.end(); i++) {
    if (!i->second->is_connected()) {
      delete i->second;
      clients.erase(i->first);
      output("removed disconnected client: " + to_string(i->first));
    }
  }

  if (clients.size() < 2) {
    sf::Packet packet;
    output("Less than two clients remaining!");
    if (!clients.empty()) {
      query_response_generator handler = [](int cid, sf::Packet q) -> handler_result {
        handler_result res;
        res.response << protocol::aborted << string("You are the last player in the game!");
        res.status = socket_t::tc_complete;
        return res;
      };

      clients.begin()->second->check_protocol(protocol::any, handler);
    }
    return false;
  }

  return true;
}

bool client_communicator::check_protocol(protocol_t p, query_response_generator f) {
  list<thread *> ts;

  for (auto c : clients) {
    ts.push_back(new thread(&client_t::check_protocol, c.second, p, f));
  }

  for (auto &t : ts) {
    t->join();
    delete t;
  }

  return cleanup_clients();
}
