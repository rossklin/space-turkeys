#include <csignal>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <mutex>
#include <thread>

#include "com_server.hpp"
#include "cost.hpp"
#include "game_handler.hpp"
#include "protocol.hpp"
#include "research.hpp"
#include "serialization.hpp"
#include "server_handler.hpp"
#include "utility.hpp"

using namespace std;
using namespace st3;
using namespace st3::server;

handler h;

void handle_sigint(int sig) {
  server::log("STARTING SHUTDOWN");
  h.handle_sigint();
  server::log("COMPLETED SHUTDOWN");
}

int main(int argc, char **argv) {
  server::log("BOOTING");
  signal(SIGINT, handle_sigint);
  try {
    game_data::confirm_data();
  } catch (parse_error &e) {
    cerr << "Parse error loading data: " << e.what() << endl;
    exit(-1);
  }

  h.run();

  return 0;
}
