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

// void end_thread(thread *&t) {
//   if (t->joinable()) t->join();
//   delete t;
//   t = 0;
// }

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

// client_communicator *handler::access_game(string gid, bool do_lock) {
//   client_communicator *g = 0;
//   if (do_lock) game_ring.lock();
//   if (games.count(gid)) {
//     g = &games[gid];
//   }
//   if (do_lock) game_ring.unlock();
//   return g;
// }

// todo: also start game monitor thread
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
    res = socket_t::tc_bad_result;
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

// // tell client to stand by until a condition is fullfilled
// void standby_until(server_cl_socket::ptr c, function<bool(void)> f) {
//   handler_result res;
//   res.status = socket_t::tc_complete;
//   res.response << protocol::standby;
//   query_response_generator handler = static_query_response(res);

//   // standby during init status
//   while (c->is_connected() && !f() && main_status == socket_t::tc_run) {
//     if (!c->check_protocol(protocol::any, handler)) {
//       c->set_disconnect();
//     }
//   }

//   // disconnect if server is shutting down
//   if (main_status != socket_t::tc_run) {
//     c->set_disconnect();
//   }

//   c->st3_state = socket_t::tc_complete;
// }

// Wait until client standby threads finish, then safely start the game handler.
// at this point, no other thread will access c -> clients
void handler::dispatch_game(game_setup gs) {
  safely([this, gs]() {
    game_data g;
    g.settings = gs.settings;
    g.build_players(hm_values(gs.clients));
    g.build();
    game_handler(gs, g);
  });

  local_output("dispatch_game: complete, disconnecting", -1, gs.id);
  gs.disconnect();
}

// // Perform client handshake, create game if necessary and add client to game
// void handler::dispatch_client(server_cl_socket::ptr c) {
//   local_output("dispatch_client: start", c->id);

//   query_response_generator client_handshake = [this, c](int cid, sf::Packet p) -> handler_result {
//     handler_result res;
//     handler_result res_invalid;
//     string gid;
//     string name;
//     client_game_settings c_settings;
//     bool test = p >> gid >> name >> c_settings;
//     local_output("join_handler: start", c->id);
//     if (!test) local_output("join_handler: failed to unpack", c->id);

//     // game_ring.lock();
//     test = test && main_status == socket_t::tc_run && valid_string(gid) && valid_string(name) && c_settings.validate();

//     res.status = socket_t::tc_complete;
//     res.response << protocol::confirm;

//     res_invalid.status = socket_t::tc_stop;
//     res_invalid.response << protocol::invalid;

//     if (!test) {
//       local_output("failed join condition", c->id, gid);
//       return res_invalid;
//     }

//     bool game_is_new = false;
//     if (!games.count(gid)) {
//       game_is_new = true;
//       create_game(gid, c_settings);
//     }

//     game_setup *cl = &games.at(gid);

//     // test if joinable
//     cl->lock();
//     if (cl->can_join()) {
//       local_output("join_handler: can join!", c->id, gid);
//       cl->add_client(c);
//       int *status = &cl->status;
//       auto condition = [status]() { return *status != socket_t::tc_init; };
//       c->wfg_thread = shared_ptr<thread>(new thread(bind(&standby_until, c, condition)));
//       c->name = name;
//       res.response << c->id;
//     } else {
//       local_output("can not join game", c->id, gid);
//       res = res_invalid;
//     }
//     cl->unlock();

//     local_output("join_handler: complete", c->id, gid);
//     return res;
//   };

//   // temporarily use server status as thread com
//   if (c->check_protocol(protocol::connect, client_handshake)) {
//     local_output("dispatch_client: starting join thread", c->id);
//   } else {
//     local_output("dispatch_client: failed protocol, deallocating", c->id);
//     c->disconnect();
//   }
// }

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
  thread t_game_dispatch(&handler::monitor_games, this);

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

  local_output("handler: terminate: waiting for threads.");
  t_game_dispatch.join();
  while (clients.size() || games.size()) {
    sf::sleep(sf::milliseconds(100));
  }
  listener.close();
  local_output("handler: completed!");
}

void handler::handle_sigint() {
  local_output("handler: caught signal SIGINT, stopping...");

  main_status = socket_t::tc_stop;

  // while (any(map<game_setup, bool>([](client_communicator g) { return g.status != socket_t::tc_complete; }, hm_values(games)))) {
  //   local_output("handler: waiting for clients to terminate...");
  //   sf::sleep(sf::milliseconds(100));
  // }

  // local_output("handler: all games terminated! Exiting.");
  // main_status = socket_t::tc_complete;
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

          if (test == socket_t::tc_run) {
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
      vector<server_cl_socket::ptr> clients = hm_values(game.clients);

      // Check if we should start loading the game
      if (game.status == socket_t::tc_init && game.clients.size() == game.settings.clset.num_players) {
        game.status = socket_t::tc_run;
      }

      // Check if clients have confirmed loading the game and we are ready to launch
      if (game.ready_to_launch()) {
        local_output("monitor games: launching game " + gid);
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
