#pragma once

#include <SFML/Network.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#include "game_settings.hpp"
#include "socket_t.hpp"
#include "types.hpp"

namespace st3 {
class game_base_data;

/*! server side specifics */
namespace server {

enum game_phase {
  SIMULATION,
  CHOICE,
  INITIALIZATION,
  NUM_PHASES
};

struct game_context {
  game_phase phase;
  mutex mtx;
  vector<unindexed_base_data> simulation_frames;
  hm_t<idtype, choice> player_choices;
  int frames_loaded;
};

typedef shared_ptr<game_context> game_context_ptr;

struct handler_result {
  sf::Packet response;
  int status;
};

typedef std::function<handler_result(int cid, sf::Packet q)> query_response_generator;

handler_result handler_switch(bool test, std::function<void(handler_result &)> on_success = 0, std::function<void(handler_result &)> on_fail = 0);

query_response_generator static_query_response(handler_result res);

/*! special socket functions for server */
struct server_cl_socket : public socket_t {
  std::string name;
  int st3_state;
  std::shared_ptr<std::thread> wfg_thread;
  std::string gid;

  server_cl_socket();
  void game_thread(game_context_ptr);
  handler_result receive_query(protocol_t p, query_response_generator f);
  bool check_protocol(protocol_t p, query_response_generator f);
  bool is_connected();
  void set_disconnect();
  bool status_is_running() override;
};

/*! structure handling communication with a set of clients associated with a game */
struct game_setup {
  hm_t<int, server_cl_socket_ptr> clients;
  game_settings settings;
  int client_idc;
  std::string id;
  int status;
  std::shared_ptr<std::thread> game_thread;
  // std::shared_ptr<std::mutex> m_lock;

  game_setup();
  void add_client(server_cl_socket_ptr c);
  void disconnect();
  bool cleanup_clients();
  bool check_protocol(protocol_t p, query_response_generator h);
  void distribute_frames(std::vector<game_base_data> &g, int &frame_count);
  // std::list<server_cl_socket *> access_clients();
  bool can_join();
  bool ready_to_launch();
  void lock();
  void unlock();
};
};  // namespace server
};  // namespace st3
