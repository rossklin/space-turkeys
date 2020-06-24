#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "client_game.h"
#include "com_client.h"
#include "cost.h"
#include "game_settings.h"
#include "graphics.h"
#include "protocol.h"
#include "research.h"
#include "selector.h"
#include "serialization.h"
#include "socket_t.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace st3::client;

void handled_response(sf::Packet p, function<void(sf::Packet)> f) {
  cout << "Attempting to send packet" << endl;
  if (!g->socket->send_packet(p)) {
    throw network_error("Failed to send packet");
  }

  cout << "Attempting to receive packet" << endl;
  if (!g->socket->receive_packet()) {
    throw network_error("Failed to receive packet");
  }

  sint r;
  if (!(g->socket->data >> r && r == protocol::confirm)) {
    throw network_error("Server did not respond with confirm");
  }

  cout << "Query complete, calling back" << endl;
  if (f) f(g->socket->data);
}

int get_status(string game_id) {
  sf::Packet p;
  p << protocol::get_status << game_id;

  int res;
  auto callback = [&res](sf::Packet p) {
    if (!(p >> res)) {
      throw network_error("Packet did not contain status");
    }
    cout << "Got game status " << res << endl;
  };

  handled_response(p, callback);
  return res;
}

void connect_to_server(string name) {
  sf::Packet p;
  p << protocol::connect << name;

  auto callback = [](sf::Packet data) {
    if (!(data >> g->socket->id)) {
      throw logical_error("Packet did not contain id");
    }
    cout << "Received client id " << g->socket->id << endl;
  };

  handled_response(p, callback);
}

string setup_create_game(client_game_settings settings) {
  sf::Packet p;
  string gid;
  p << protocol::create_game << settings;

  auto callback = [&gid](sf::Packet p) {
    if (!(p >> gid)) {
      throw logical_error("Packet did not contain gid");
    }
    cout << "Created game " << gid << endl;
  };

  handled_response(p, callback);
  return gid;
}

void setup_join_game(string gid) {
  sf::Packet p;
  p << protocol::join_game << gid;
  handled_response(p, 0);
  cout << "Joined game " << gid << endl;
}

void setup_gfx(bool fullscreen = false) {
  int width = 800;
  int height = 600;
  sf::VideoMode vmode(width, height);
  sf::Uint32 vstyle = sf::Style::Default;

  if (fullscreen) {
    vmode = sf::VideoMode::getDesktopMode();
    vstyle = sf::Style::Fullscreen;
  }

  // create graphics

  sf::ContextSettings sf_settings;
  sf_settings.antialiasingLevel = 8;
  g->window.create(vmode, "SPACE TURKEYS III ALPHA", vstyle, sf_settings);
}

int main(int argc, char **argv) {
  string game_id = "";
  string ip = "127.0.0.1";
  string name = "Name_blabla";
  string task = "create";
  client_game_settings settings;
  bool fullscreen = false;

  game_data::confirm_data();

  name[utility::random_int(name.length())] = utility::random_int(256);
  name[utility::random_int(name.length())] = utility::random_int(256);
  name[utility::random_int(name.length())] = utility::random_int(256);

  auto parse_input = [&](string x) {
    size_t idx = x.find("=");
    if (idx == string::npos) {
      throw classified_error("Invalid input: " + x);
    }

    string key = x.substr(0, idx);
    string value = x.substr(idx + 1);

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
        throw classified_error("Invalid starting fleet option: " + value);
      }
    } else if (key == "skip_sub") {
      sub_frames = 1;
    } else if (key == "restart") {
      settings.restart = stoi(value);
    } else if (key == "task") {
      task = value;
    } else {
      throw classified_error("Invalid input: " + x);
    }
  };

  try {
    for (int i = 1; i < argc; i++) parse_input(argv[i]);
  } catch (exception e) {
    cout << e.what() << endl;
    cout << "usage: " << argv[0] << " [task={create|join}] [game_id=...] [ip=...] [name=...] [num_players=...] [size=...] [round_length=...] [starting_fleet=single|voyagers|battleships|massive]" << endl;
    exit(0);
  }

  game g_obj;
  g = &g_obj;

  // connect
  cout << "connecting...";
  g->socket = new cl_socket_t();
  if (g->socket->connect(ip, 53000) != sf::Socket::Done) {
    cout << "client: connection failed." << endl;
    return -1;
  }
  g->socket->setBlocking(false);
  cout << "done." << endl;

  try {
    setup_gfx(fullscreen);

    sfg::SFGUI sfgui;
    g->sfgui = &sfgui;
    graphics::initialize();

    connect_to_server(name);
    if (task == "create") {
      game_id = setup_create_game(settings);
    } else if (task == "join" && game_id.size()) {
      setup_join_game(game_id);
    } else {
      throw logical_error("Invalid task or missing game id");
    }

    while (get_status(game_id) != socket_t::tc_ready_game) {
      sf::sleep(sf::milliseconds(1000));
    }

    cout << "Calling g->run" << endl;

    g->run();

  } catch (exception &e) {
    cout << "Exception in client::run: " << e.what() << endl;
    cout << "Exiting..." << endl;
  }

  g->socket->disconnect();
  delete g->socket;

  return 0;
}
