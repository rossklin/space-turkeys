#ifndef _STK_SERVER_HANDLER
#define _STK_SERVER_HANDLER

#include <SFML/Network.hpp>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "com_server.h"
#include "game_settings.h"
#include "types.h"

namespace st3 {
namespace server {
struct handler {
  hm_t<std::string, com> games;
  int status;
  std::mutex game_ring;
  static void safely(std::function<void()> f, std::function<void()> on_fail = 0);

  handler();
  void run();
  void game_dispatcher();
  void dispatch_game(std::string gid);
  void dispatch_client(client_t *c);
  void wfg(client_t *c);
  void handle_sigint();
  com *access_game(std::string gid, bool do_lock = true);
  com *create_game(std::string gid, client_game_settings s, bool do_lock = true);
};
};  // namespace server
};  // namespace st3

#endif
