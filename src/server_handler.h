#ifndef _STK_SERVER_HANDLER
#define _STK_SERVER_HANDLER

#include <string>
#include <thread>
#include <mutex>
#include <SFML/Network.hpp>

#include "com_server.h"

namespace server {
  struct handler {
    hm_t<string, com> games;
    hm_t<string, thread*> threads;
    int status;
    mutex game_ring;

    handler();  
    void run();
    void game_dispatcher();
    void dispatch_game(com &c);
    void dispatch_client(client_t *c);
    void wait_for_game(client_t *c);
    void handle_sigint();
  };
};

#endif
