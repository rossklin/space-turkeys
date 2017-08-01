#ifndef _STK_SERVER_HANDLER
#define _STK_SERVER_HANDLER

#include <string>
#include <thread>
#include <mutex>
#include <SFML/Network.hpp>

#include "types.h"
#include "com_server.h"
#include "game_settings.h"

namespace st3 {
  namespace server {
    struct handler {
      hm_t<std::string, com> games;
      int status;
      std::mutex game_ring;
      std::mutex status_ring;

      handler();  
      void run();
      void game_dispatcher();
      void dispatch_game(std::string gid);
      void dispatch_client(client_t *c);
      void wfg(client_t *c);
      void handle_sigint();
      com *access_game(std::string gid);
      com *create_game(std::string gid, client_game_settings s);
    };
  };
};

#endif
