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

#include "client_game.hpp"
#include "com_client.hpp"
#include "cost.hpp"
#include "game_settings.hpp"
#include "graphics.hpp"
#include "protocol.hpp"
#include "research.hpp"
#include "selector.hpp"
#include "serialization.hpp"
#include "socket_t.hpp"
#include "utility.hpp"

using namespace std;
using namespace st3;

void handled_response(cl_socket_ptr socket, sf::Packet p, function<void(sf::Packet)> f) {
  cout << "Attempting to send packet" << endl;
  if (!socket->send_packet(p)) {
    throw network_error("Failed to send packet");
  }

  cout << "Attempting to receive packet" << endl;
  if (!socket->receive_packet()) {
    throw network_error("Failed to receive packet");
  }

  sint r;
  if (!(socket->data >> r && r == protocol::confirm)) {
    throw network_error("Server did not respond with confirm");
  }

  cout << "Query complete, calling back" << endl;
  if (f) f(socket->data);
}

int get_status(cl_socket_ptr socket, string game_id) {
  sf::Packet p;
  p << protocol::get_status << game_id;

  int res;
  auto callback = [&res](sf::Packet p) {
    if (!(p >> res)) {
      throw network_error("Packet did not contain status");
    }
    cout << "Got game status " << res << endl;
  };

  handled_response(socket, p, callback);
  return res;
}

void connect_to_server(cl_socket_ptr socket, string name) {
  sf::Packet p;
  p << protocol::connect << name;

  auto callback = [socket](sf::Packet data) {
    if (!(data >> socket->id)) {
      throw logical_error("Packet did not contain id");
    }
    cout << "Received client id " << socket->id << endl;
  };

  handled_response(socket, p, callback);
}

string setup_create_game(cl_socket_ptr socket, client_game_settings settings) {
  sf::Packet p;
  string gid;
  p << protocol::create_game << settings;

  auto callback = [&gid](sf::Packet p) {
    if (!(p >> gid)) {
      throw logical_error("Packet did not contain gid");
    }
    cout << "Created game " << gid << endl;
  };

  handled_response(socket, p, callback);
  return gid;
}

void setup_join_game(cl_socket_ptr socket, string gid) {
  sf::Packet p;
  p << protocol::join_game << gid;
  handled_response(socket, p, 0);
  cout << "Joined game " << gid << endl;
}

RSG::WindowPtr setup_gfx(bool fullscreen = false) {
  int width = 1024;
  int height = 9 * 1024 / 16;
  sf::VideoMode vmode(width, height);
  sf::Uint32 vstyle = sf::Style::Default;

  if (fullscreen) {
    vmode = sf::VideoMode::getDesktopMode();
    vstyle = sf::Style::Fullscreen;
  }

  // create graphics

  sf::ContextSettings sf_settings;
  sf_settings.antialiasingLevel = 8;
  RSG::WindowPtr w = shared_ptr<window_t>(new window_t());
  w->create(vmode, "SPACE TURKEYS III ALPHA", vstyle, sf_settings);
  return w;
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
      settings.sim_sub_frames = 1;
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

  // connect
  cout << "connecting...";
  shared_ptr<cl_socket_t> socket(new cl_socket_t());
  if (socket->connect(ip, 53000) != sf::Socket::Done) {
    cout << "client: connection failed." << endl;
    return -1;
  }
  socket->setBlocking(false);
  socket->instruction = socket_t::tc_run;
  cout << "done." << endl;

  try {
    auto win = setup_gfx(fullscreen);

    connect_to_server(socket, name);
    if (task == "create") {
      game_id = setup_create_game(socket, settings);
    } else if (task == "join" && game_id.size()) {
      setup_join_game(socket, game_id);
    } else {
      throw logical_error("Invalid task or missing game id");
    }

    while (get_status(socket, game_id) != socket_t::tc_ready_game) {
      sf::sleep(sf::milliseconds(1000));
    }

    cout << "Calling run" << endl;

    shared_ptr<game> g(new game(socket, win));
    game::gmain = g;
    g->run();

  } catch (exception &e) {
    cout << "Exception in client::run: " << e.what() << endl;
    cout << "Exiting..." << endl;
  }

  socket->disconnect();

  return 0;
}
