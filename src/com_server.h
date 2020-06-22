#pragma once

#include <SFML/Network.hpp>
#include <functional>
#include <list>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "game_settings.h"
#include "socket_t.h"
#include "types.h"

namespace st3 {
class entity_package;

/*! server side specifics */
namespace server {
struct handler_result {
  sf::Packet response;
  int status;
};

typedef std::function<handler_result(int cid, sf::Packet q)> query_handler;

handler_result handler_switch(bool test, std::function<void(handler_result &)> on_success = 0, std::function<void(handler_result &)> on_fail = 0);

/*! special socket functions for server */
struct client_t : public socket_t {
  std::string name;
  std::string game_id;
  std::thread *wfg_thread;

  handler_result receive_query(protocol_t p, query_handler f);
  bool check_protocol(protocol_t p, query_handler f);
  bool is_connected();
  void set_disconnect();
};

/*! structure handling a set of clients */
struct com {
  hm_t<int, client_t *> clients;
  game_settings settings;
  int idc;
  int thread_com;
  std::string gid;
  std::mutex m_lock;
  std::thread *active_thread;

  com();
  void add_client(client_t *c);
  void disconnect();
  bool cleanup_clients();
  bool check_protocol(protocol_t p, query_handler h);
  void distribute_frames(std::vector<entity_package> &g, int &frame_count);
  std::list<client_t *> access_clients();
  bool can_join();
  bool ready_to_launch();
  void lock();
  void unlock();
};
};  // namespace server
};  // namespace st3
