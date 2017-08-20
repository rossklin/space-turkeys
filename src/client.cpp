#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <thread>
#include <ctime>
#include <set>

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
#include "serialization.h"
#include "game_settings.h"

using namespace std;
using namespace st3;
using namespace st3::client;

void run(string game_id, string name, client_game_settings settings, bool fullscreen = false) {  
  int width = 800;
  int height = 600;
  sf::VideoMode vmode(width, height);
  sf::Uint32 vstyle = sf::Style::Default;

  if (fullscreen) {
    auto test = sf::VideoMode::getFullscreenModes();
    sort(test.begin(), test.end(), [] (sf::VideoMode a, sf::VideoMode b) {return a.width > b.width;});
    vmode = test.front();
    vstyle = sf::Style::Fullscreen;
  }

  // create graphics
  sf::Packet pq, pr;
  int tc_in = socket_t::tc_run;
  int tc_out = 0;

  cout << "sending game id..." << endl;
  pq << protocol::connect << game_id << name << settings;
  query(g -> socket, pq, tc_in, tc_out);

  if (tc_out & socket_t::tc_bad_result) {
    throw runtime_error("Failed to send game id to server.");
  }

  if (!(g -> socket -> data >> g -> socket -> id)){
    throw runtime_error("server failed to provide id");
  }

  cout << "received player id: " << g -> socket -> id << endl;

  sf::ContextSettings sf_settings;
  sf_settings.antialiasingLevel = 8;
  g -> window.create(vmode, "SPACE TURKEYS III ALPHA", vstyle, sf_settings);

  sfg::SFGUI sfgui;

  g -> sfgui = &sfgui;
  g -> window.setActive();  
  
  graphics::initialize();

  g -> run();

}

int main(int argc, char **argv){
  string game_id = "game1";
  string ip = "127.0.0.1";
  string name = "Name_blabla";
  client_game_settings settings;
  bool fullscreen = false;

  game_data::confirm_data();

  name[utility::random_int(name.length())] = utility::random_int(256);
  name[utility::random_int(name.length())] = utility::random_int(256);
  name[utility::random_int(name.length())] = utility::random_int(256);

  auto parse_input = [&game_id, &ip, &name, &settings, &fullscreen] (string x) {
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
    } else if (key == "fullscreen") {
      fullscreen = stoi(value);
    } else if (key == "num_players") {
      settings.num_players = stoi(value);
    } else if (key == "size") {
      settings.galaxy_radius = stof(value);
    } else if (key == "round_length") {
      settings.frames_per_round = stoi(value);
    } else if (key == "starting_fleet") {
      if (starting_options.count(value)) {
	settings.starting_fleet = value;
      } else {
	throw runtime_error("Invalid starting fleet option: " + value);
      }
    } else {
      throw runtime_error("Invalid input: " + x);
    }
  };

  try {
    for (int i = 1; i < argc; i++) parse_input(argv[i]);
  } catch (exception e) {
    cout << e.what() << endl;
    cout << "usage: " << argv[0] << " [game_id=...] [ip=...] [name=...] [num_players=...] [size=...] [round_length=...] [starting_fleet=single|voyagers|massive]" << endl;
    exit(0);
  }

  game g_obj;
  g = &g_obj;

  // connect
  cout << "connecting...";
  g -> socket = new socket_t();
  if (g -> socket -> connect(ip, 53000) != sf::Socket::Done){
    cout << "client: connection failed." << endl;
    return -1;
  }
  g -> socket -> setBlocking(false);
  cout << "done." << endl;

  try {
    run(game_id, name, settings, fullscreen);
  } catch (exception e) {
    cout << "Exception in client::run: " << e.what() << endl;
    cout << "Exiting..." << endl;
  }

  g -> socket -> disconnect();
  delete g -> socket;

  return 0;
}
