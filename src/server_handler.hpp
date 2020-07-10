#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "com_server.h"
#include "game_settings.h"
#include "types.h"

namespace st3 {
namespace server {
void safely(std::function<void()> f, std::function<void()> on_fail = 0);
extern int main_status;

struct handler {
  hm_t<int, server_cl_socket_ptr> clients;
  hm_t<std::string, game_setup> games;
  std::mutex game_ring;
  int idc;

  handler();
  void run();
  void main_client_handler(server_cl_socket_ptr cl);
  void monitor_games();
  // void game_dispatcher();
  void dispatch_game(game_setup gs);
  // void dispatch_client(server_cl_socket_ptr c);
  // void wfg(server_cl_socket *c);
  void handle_sigint();
  // client_communicator *access_game(std::string gid, bool do_lock = true);
  std::string create_game(client_game_settings s);
  bool join_game(server_cl_socket_ptr cl, std::string gid);
  sint get_status(std::string gid);
  void disconnect(server_cl_socket_ptr cl);
};
};  // namespace server
};  // namespace st3
