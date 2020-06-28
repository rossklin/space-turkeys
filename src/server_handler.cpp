#include "server_handler.h"

#include <fstream>
#include <iostream>
#include <sstream>  // stringstream
#include <string>   // string

#include "game_data.h"
#include "game_handler.h"
#include "protocol.h"
#include "serialization.h"
#include "types.h"

using namespace std;
using namespace st3;
using namespace server;

int st3::server::main_status;

// show server handler output even though DEBUG is not defined
void local_output(string v, int cid = -1, string gid = "") {
  string mes = "handler: ";
  if (cid > -1) mes += "client " + to_string(cid);
  if (gid.size()) mes += " (game " + gid + ")";
  mes += ": " + v;

  output(mes, true);
  server::log(mes);
}

bool valid_string(string v) {
  // todo
  return v.length() < 1000;
}

void server::safely(function<void()> f, function<void()> g) {
  try {
    f();
  } catch (classified_error &e) {
    server::log(e.what(), e.severity);
    if (g) g();
  } catch (exception &e) {
    server::log(e.what(), "unknown exception");
    if (g) g();
  } catch (...) {
    server::log("unknown error", "unknown error");
    if (g) g();
  }
}

handler::handler() {
  idc = 1;
}

string handler::create_game(client_game_settings set) {
  game_ring.lock();
  game_setup gs;
  gs.id = to_string(idc++);
  gs.settings.clset = set;
  games[gs.id] = gs;
  game_ring.unlock();
  return gs.id;
}

bool handler::join_game(server_cl_socket::ptr cl, string gid) {
  bool res = true;
  game_ring.lock();
  if (!games.count(gid)) res = false;
  if (!games.at(gid).can_join()) res = false;
  if (res) {
    games.at(gid).add_client(cl);
    cl->gid = gid;
  }
  game_ring.unlock();
  return res;
}

sint handler::get_status(string gid) {
  sint res;
  game_ring.lock();
  if (games.count(gid)) {
    res = games.at(gid).status;
  } else {
    res = socket_t::tc_failed;
  }
  game_ring.unlock();

  return res;
}

void handler::disconnect(server_cl_socket::ptr cl) {
  game_ring.lock();
  if (games.count(cl->gid)) {
    games.at(cl->gid).clients.erase(cl->id);
  }
  game_ring.unlock();
}

// Wait until client standby threads finish, then safely start the game handler.
// at this point, no other thread will access c -> clients
void handler::dispatch_game(game_setup gs) {
  safely([this, gs]() {
    game_data g;
    g.settings = gs.settings;
    g.build_players(utility::hm_values(gs.clients));
    g.build();
    game_handler(gs, g);
  });

  local_output("dispatch_game: complete, disconnecting", -1, gs.id);
  gs.disconnect();
}

// Start game dispatcher, listen for clients and dispatch them
void handler::run() {
  main_status = socket_t::tc_run;
  sf::TcpListener listener;

  local_output("Binding listener...");
  // bind the listener to a port
  if (listener.listen(53000) != sf::Socket::Done) {
    local_output("Failed to bind listener!");
    return;
  }
  listener.setBlocking(false);

  // start worker threads
  thread t_monitor_games(&handler::monitor_games, this);

  // accept connections
  while (main_status == socket_t::tc_run) {
    server_cl_socket::ptr test(new server_cl_socket());
    sf::Socket::Status code;

    while ((code = listener.accept(*test)) == sf::Socket::Partial) {
    }

    if (code == sf::Socket::Done) {
      local_output("Accepted a new client");
      test->wfg_thread = shared_ptr<thread>(new thread(&handler::main_client_handler, this, test));
      clients[test->id] = test;
    } else {
      test->disconnect();
    }

    sf::sleep(sf::milliseconds(500));
  }

  local_output("terminate: waiting for connections to close.");
  listener.close();
  while (clients.size() || games.size()) {
    sf::sleep(sf::milliseconds(100));
  }
  local_output("terminate: waiting for game monitor to finish.");
  main_status = socket_t::tc_complete;
  t_monitor_games.join();
  local_output("terminate: completed!");
}

void handler::handle_sigint() {
  local_output("handler: caught signal SIGINT, stopping...");
  main_status = socket_t::tc_stop;
}

// todo: handle starting game
void handler::main_client_handler(server_cl_socket::ptr cl) {
  bool run = true;

  cout << "Main client handler: starting for client " << cl->id << endl;

  while (run && cl->receive_packet() && cl->is_connected()) {
    cout << "Main client handler: packet received for " << cl->id << endl;
    int q;
    string gid;
    client_game_settings clset;
    sf::Packet res;

    if (!(cl->data >> q)) {
      q = protocol::invalid;
    }

    switch (q) {
      case protocol::connect:
        if (cl->data >> cl->name && valid_string(cl->name)) {
          res << protocol::confirm << cl->id;
          cout << "Sending id " << cl->id << " to client" << endl;
        } else {
          res << protocol::invalid;
        }
        break;
      case protocol::create_game:
        if (cl->data >> clset) {
          gid = create_game(clset);
          if (join_game(cl, gid)) {
            res << protocol::confirm << gid;
          } else {
            res << protocol::invalid;
          }
        } else {
          res << protocol::invalid;
        }
        break;
      case protocol::join_game:
        if ((cl->data >> gid) && join_game(cl, gid)) {
          res << protocol::confirm;
        } else {
          res << protocol::invalid;
        }
        break;

      case protocol::get_status:
        if (cl->data >> gid) {
          sint test = get_status(gid);
          res << protocol::confirm << test;

          if (test == socket_t::tc_ready_game) {
            // The game is starting so end this thread
            cl->st3_state = socket_t::tc_run;
            run = false;
          }
        }
        break;

      case protocol::disconnect:
        disconnect(cl);
        res << protocol::confirm;
        run = false;
        break;

      default:
        res << protocol::invalid;
        break;
    }

    cl->send_packet(res);
    cout << "Main client handler: packet sent to " << cl->id << endl;
  }

  // disconnect unless client is being passed to a game
  if (cl->st3_state != socket_t::tc_run) {
    cl->disconnect();
    cl->st3_state = socket_t::tc_complete;
  }
}

// todo
void handler::monitor_games() {
  list<string> rbuf;
  local_output("monitor games: start");

  while (main_status != socket_t::tc_complete) {
    rbuf.clear();

    // Identify clients that completed main handler and should be removed
    vector<int> clrbuf;
    for (auto x : clients) {
      server_cl_socket::ptr cl = x.second;
      if (cl->st3_state == socket_t::tc_complete) clrbuf.push_back(cl->id);
    }

    game_ring.lock();

    // Cleanupt client main threads and remove clients from games
    for (int cid : clrbuf) {
      clients[cid]->wfg_thread->join();
      if (clients[cid]->gid.size()) {
        games[clients[cid]->gid].clients.erase(cid);
      }
      clients.erase(cid);
    }

    // check games to launch
    for (auto &c : games) {
      game_setup &game = c.second;
      string gid = c.first;
      vector<server_cl_socket::ptr> clients = utility::hm_values(game.clients);

      // Check if we should start loading the game
      if (game.status == socket_t::tc_init && game.clients.size() == game.settings.clset.num_players) {
        game.status = socket_t::tc_ready_game;
      }

      // Check if clients have confirmed loading the game and we are ready to launch
      if (game.ready_to_launch()) {
        local_output("monitor games: launching game " + gid);
        game.status = socket_t::tc_run;
        game.game_thread = shared_ptr<thread>(new thread(&handler::dispatch_game, this, game));
      }

      // Remove games that do not have any clients
      if (game.clients.empty()) {
        local_output("monitor games: game " + gid + " has no clients, listing for removal.");
        rbuf.push_back(gid);
      }
    }

    // remove games
    for (auto v : rbuf) {
      local_output("monitor games: register game " + v + " for termination.");
      if (games[v].game_thread) games[v].game_thread->join();
      games.erase(v);
      local_output("monitor games: completed terminating game " + v);
    }

    game_ring.unlock();
    sf::sleep(sf::milliseconds(100));
  }

  local_output("monitor games: end");
}
