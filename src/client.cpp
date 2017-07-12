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
  string nump = "2";

  game_data::confirm_data();
  utility::init();

  name[utility::random_int(name.length())] = utility::random_int(256);
  name[utility::random_int(name.length())] = utility::random_int(256);
  name[utility::random_int(name.length())] = utility::random_int(256);

  auto parse_input = [&game_id, &ip, &name, &nump] (string x) {
    size_t idx = x.find("=");
    if (idx == string::npos) {
      throw runtime_error("Invalid input: " + x);
    }

    string key = x.substr(0, idx);
    string value = x.substr(idx+1);

    if (key == "game_id") {
      game_id = value;
    } else if (key == "ip") {
      ip = value;
    } else if (key == "name") {
      name = value;
    } else if (key == "num_players") {
      nump = value;
    } else {
      throw runtime_error("Invalid input: " + x);
    }
  };

  try {
    for (int i = 1; i < argc; i++) parse_input(argv[i]);
  } catch (exception e) {
    cout << e.what() << endl;
    cout << "usage: " << argv[0] << " [game_id=...] [ip=...] [name=...] [num_players=...]" << endl;
    exit(0);
  }

  game_id += ":" + nump;
  
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
