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

  c.connect(num_clients);
  c.introduce();

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
  
  game_handler(c, g);

  c.disconnect();

  return 0;
}
