#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <thread>
#include <ctime>

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

#include "socket_t.h"
#include "graphics.h"
#include "client_game.h"
#include "com_client.h"
#include "protocol.h"
#include "cost.h"
#include "research.h"
#include "selector.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace st3::client;

int main(int argc, char **argv){
  int width = 800;
  int height = 600;
  string game_id = "game1";
  string ip = "127.0.0.1";
  string name = "Name_blabla";
  int nump = 2;

  game_data::confirm_data();
  utility::init();

  name[utility::random_int(name.length())] = utility::random_int(256);
  name[utility::random_int(name.length())] = utility::random_int(256);
  name[utility::random_int(name.length())] = utility::random_int(256);

  if (argc > 5){
    cout << "usage: " << argv[0] << " [game_id] [ip_number] [name] [num_players]" << endl;
    exit(0);
  }

  if (argc > 4) nump = atoi(argv[4]);
  if (argc > 3) name = argv[3];
  if (argc > 2) ip = argv[2];
  if (argc > 1) game_id = argv[1];

  game_id += ":" + to_string(nump);
  
  // connect
  cout << "connecting...";
  game g;
  g.socket = new socket_t();
  if (g.socket -> connect(ip, 53000) != sf::Socket::Done){
    cout << "client: connection failed." << endl;
    return -1;
  }
  g.socket -> setBlocking(false);
  cout << "done." << endl;

  // create graphics
  sf::Packet pq, pr;
  int done;

  cout << "sending game id..." << endl;
  pq << protocol::connect << game_id << nump;
  query(g.socket, pq, done);

  cout << "sending name..." << endl;
  pq.clear();
  pq << protocol::id << name;
  query(g.socket, pq, done);

  if (!(g.socket -> data >> g.socket -> id)){
    throw runtime_error("server failed to provide id");
  }

  cout << "received player id: " << g.socket -> id << endl;

  sf::ContextSettings settings;
  settings.antialiasingLevel = 8;
  g.window.create(sf::VideoMode(width, height), "SFML Turkeys!", sf::Style::Default, settings);

  sfg::SFGUI sfgui;

  g.sfgui = &sfgui;
  g.window.setActive();
  
  graphics::initialize();
  // todo: might need to g.window.setActive(), and might not even be
  // able to construct sfgui in g before g.window.create()

  entity_selector::g = &g;
  g.run();

  g.socket -> disconnect();
  delete g.socket;

  return 0;
}
