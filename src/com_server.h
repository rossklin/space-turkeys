#pragma once

#include <SFML/Network.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

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

typedef std::function<handler_result(int cid, sf::Packet q)> query_response_generator;

handler_result handler_switch(bool test, std::function<void(handler_result &)> on_success = 0, std::function<void(handler_result &)> on_fail = 0);

query_response_generator static_query_response(handler_result res);

/*! special socket functions for server */
struct server_cl_socket : public socket_t {
  typedef std::shared_ptr<server_cl_socket> ptr;

  std::string name;
  int st3_state;
  std::shared_ptr<std::thread> wfg_thread;
  std::string gid;

  server_cl_socket();
  handler_result receive_query(protocol_t p, query_response_generator f);
  bool check_protocol(protocol_t p, query_response_generator f);
  bool is_connected();
  void set_disconnect();
  bool check_com() override;
};

/*! structure handling communication with a set of clients associated with a game */
struct game_setup {
  hm_t<int, server_cl_socket::ptr> clients;
  game_settings settings;
  int client_idc;
  std::string id;
  int status;
  std::shared_ptr<std::thread> game_thread;
  // std::shared_ptr<std::mutex> m_lock;

  game_setup();
  void add_client(server_cl_socket::ptr c);
  void disconnect();
  bool cleanup_clients();
  bool check_protocol(protocol_t p, query_response_generator h);
  void distribute_frames(std::vector<entity_package> &g, int &frame_count);
  // std::list<server_cl_socket *> access_clients();
  bool can_join();
  bool ready_to_launch();
  void lock();
  void unlock();
};
};  // namespace server
};  // namespace st3
