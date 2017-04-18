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

  // build player data
  vector<sint> colbuf = utility::different_colors(num_clients);
  int i = 0;
  for (auto x : c.clients){
    player p;
    p.name = x.second -> name;
    p.color = colbuf[i++];
    g.players[x.first] = p;
  }

  g.build();

  try {
    game_handler(c, g);
  } catch (...) {
    c.disconnect();
  }

  c.disconnect();

  return 0;
}
