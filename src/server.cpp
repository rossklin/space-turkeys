#include <iostream>
#include <exception>
#include <cstdlib>
#include <thread>
#include <csignal>
#include <mutex>

#include "game_handler.h"
#include "com_server.h"
#include "protocol.h"
#include "utility.h"
#include "cost.h"
#include "research.h"
#include "serialization.h"
#include "server_handler.h"

using namespace std;
using namespace st3;
using namespace st3::server;

handler h;

void handle_sigint(int sig) {
  h.handle_sigint();
}

int main(int argc, char **argv){
  signal(SIGINT, handle_sigint);
  game_data::confirm_data();
  utility::init();
  h.run();

  return 0;
}
