#include <iostream>

#include "game_handler.h"
#include "com_server.h"
#include "protocol.h"
#include "utility.h"
#include "cost.h"
#include "research.h"

using namespace std;
using namespace st3;
using namespace st3::server;

int main(int argc, char **argv){
  sf::TcpListener listener;
  int num_clients = argc == 2 ? atoi(argv[1]) : 2;

  srand(time(NULL));

  com c;
  game_data g;

  if (!(c.connect(num_clients) && c.introduce())){
    cout << "failed to build connections" << endl;
    exit(-1);
  }

  g.build_players(c.clients);
  g.build();

  try {
    game_handler(c, g);
  } catch (...) {
    c.disconnect();
  }

  c.disconnect();

  return 0;
}
