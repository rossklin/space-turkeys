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

using namespace std;
using namespace st3;
using namespace st3::client;

int main(int argc, char **argv){
  int width = 800;
  int height = 600;
  string ip = "127.0.0.1";
  string name = "Name_blabla";

  srand(time(NULL));

  name[rand()%name.length()] = rand() % 256;
  name[rand()%name.length()] = rand() % 256;
  name[rand()%name.length()] = rand() % 256;

  if (argc > 3){
    cout << "usage: " << argv[0] << " [ip_number] [name]" << endl;
    exit(-1);
  }else if (argc == 3){
    ip = argv[1];
    name = argv[2];
  }else if (argc == 2){
    ip = argv[1];
  }

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

  cout << "sending name...";
  pq << protocol::connect << name;

  query(g.socket, pq, done);

  if (!(g.socket -> data >> g.socket -> id)){
    cout << "server failed to provide id" << endl;
    exit(-1);
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
  
  g.run();

  g.socket -> disconnect();
  delete g.socket;

  return 0;
}
