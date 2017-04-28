#include <iostream>
#include <exception>
#include <cstdlib>

#include "game_handler.h"
#include "com_server.h"
#include "protocol.h"
#include "utility.h"
#include "cost.h"
#include "research.h"

using namespace std;
using namespace st3;
using namespace st3::server;

void on_terminate() {
  abort();
}

int main(int argc, char **argv){
  sf::TcpListener listener;
  int num_clients = argc == 2 ? atoi(argv[1]) : 2;

  srand(time(NULL));

  com c;
  game_data g;

  if (!(c.connect(num_clients) && c.introduce())){
    throw runtime_error("failed to build connections");
  }

  set_terminate(on_terminate);

  utility::init();
  g.build_players(c.clients);
  g.build();

  try {
    game_handler(c, g);
  } catch (exception &e) {
    cout << "Exception in game_handler: " << e.what() << endl;
    c.disconnect();
  }

  c.disconnect();

  return 0;
}
