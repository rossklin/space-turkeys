#include "game_handler.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "com_server.hpp"
#include "game_data.hpp"
#include "protocol.hpp"
#include "serialization.hpp"
#include "server_handler.hpp"
#include "solar.hpp"
#include "utility.hpp"

using namespace std;
using namespace st3;
using namespace st3::server;
using namespace st3::utility;

// limit_to without deallocating
void limit_to(game_base_data *g, set<idtype> ids) {
  list<combid> remove_buf;
  for (auto e : g->all_entities<game_object>()) {
    bool test = any(
        range_map<list<bool>>([g, e](idtype id) {
          bool known = e->isa(solar::class_id) && guaranteed_cast<solar>(e)->known_by.count(id);
          bool owned = e->owner == id;
          bool seen = g->evm[id].count(e->id);
          return known || owned || seen;
        },
                              ids));
    if (!test) remove_buf.push_back(e->id);
  }
  for (auto i : remove_buf) g->remove_entity(i);
}

void simulation_step(game_setup &c, game_data &g) {
  int n = g.settings.clset.frames_per_round;
  vector<game_base_data> frames(n);
  int frame_count = 0;

  query_response_generator handler = [&c, &frames, &frame_count](int cid, sf::Packet q) -> handler_result {
    int idx;
    bool test = q >> idx;

    auto on_success = [&frames, &frame_count, idx, cid](handler_result &result) {
      game_base_data g;

      if (idx < 0) {
        // client done - leave default confirm status
      } else if (idx < frame_count) {
        // pack frame
        g = frames[idx];
        limit_to(&g, {cid});
        result.response << g;
        result.status = socket_t::tc_run;
      } else if (idx < frames.size()) {
        // tell client to stand by
        result.response.clear();
        result.response << protocol::standby;
        result.status = socket_t::tc_run;
      } else {
        // invalid idx
        result.response.clear();
        result.response << protocol::invalid;
        result.status = socket_t::tc_stop;
      }
    };

    return handler_switch(test, on_success);
  };

  auto generate_frames = [&c, &g, &frames, &frame_count, n]() {
    auto task = [&]() {
      for (frame_count = 0; frame_count < n; frame_count++) {
        g.increment();
        frames[frame_count].copy_from(g);
        if (main_status != socket_t::tc_run) break;
      }
    };

    safely(task, [&]() { c.status = socket_t::tc_stop; });
  };

  thread t(generate_frames);
  if (!c.check_protocol(protocol::frame, handler)) c.status = socket_t::tc_stop;
  t.join();
}

bool check_end(game_setup &c, game_data &g) {
  if (main_status != socket_t::tc_run) {
    server::log("game_handler::check_end: server shut down!");
    return true;
  }

  int pid = -1;
  int psum = 0;
  for (auto x : g.filtered_entities<solar>()) {
    if (x->owner > -1 && x->owner != pid) {
      pid = x->owner;
      psum++;
    }
  }

  if (psum < 2) {
    query_response_generator h = [&c, psum, pid](int cid, sf::Packet data) -> handler_result {
      handler_result res;
      string message;

      if (psum == 1) {
        if (cid == pid) {
          message = "You won the game!";
        } else {
          message = "Player " + to_string(pid) + ": " + c.clients[pid]->name + " won the game.";
        }
      } else {
        message = "The game is a tie.";
      }

      res.response << protocol::complete << message;
      res.status = socket_t::tc_complete;
      return res;
    };

    c.check_protocol(protocol::game_round, h);
    return true;
  }

  return false;
}

query_response_generator pack_game_handler(game_data &g, bool do_limit) {
  return [&g, do_limit](int cid, sf::Packet query) -> handler_result {
    handler_result res;
    game_base_data ep = g;
    set<int> all_ids = range_init<set<int>>(hm_keys(g.players));

    if (do_limit) {
      limit_to(&ep, {cid});
    } else {
      limit_to(&ep, all_ids);
    }

    res.response << protocol::confirm << ep;
    res.status = socket_t::tc_complete;

    return res;
  };
}

void autosave_game(game_setup c, game_data &g) {
  if (c.id.empty()) {
    throw classified_error("autosave: server com object missing gid!");
  }

  game_base_data ep = g;
  sf::Packet p;

  if (!(p << ep)) {
    throw classified_error("autosave: failed to serialize!");
  }

  const void *data = p.getData();
  int n = p.getDataSize();

  string filename = "save/" + c.id + ".auto.save";
  ofstream of(filename, ios::binary);
  of.write((const char *)data, n);
  bool success = !of.fail();
  of.close();

  if (!success) {
    throw classified_error("autosave: failed to write to file!");
  }
}

bool load_autosave(string filename, game_setup &c, game_data &g) {
  ifstream file("save/" + filename, ios::binary | ios::ate);

  if (file.good() && !g.settings.clset.restart) {
    streamsize size = file.tellg();
    file.seekg(0, ios::beg);
    vector<char> buffer(size);
    if (file.read(buffer.data(), size)) {
      sf::Packet p;
      p.append((const void *)buffer.data(), size);

      game_base_data ep;
      if (!(p >> ep)) {
        throw classified_error("autoload: failed to deserialize!");
      }

      // map players by name
      hm_t<int, server_cl_socket_ptr> new_clients;
      for (auto x : g.players) {
        bool success = false;
        for (auto y : ep.players) {
          if (x.second.name == y.second.name) {
            if (new_clients.count(y.first)) {
              throw classified_error("autoload: attempted to register multiple players on the same name!");
            }

            new_clients[y.first] = c.clients[x.first];
            new_clients[y.first]->id = y.first;
            server::log("autoload: mapped player " + y.second.name + " from id " + to_string(x.first) + " to id " + to_string(y.first));
            success = true;
          }
        }

        if (!success) {
          throw classified_error("Failed to map id for player " + to_string(x.first));
        }
      }

      c.clients = new_clients;
      g.copy_from(ep);

      return true;
    }
  }

  return false;
}

void server::game_handler(game_setup c, game_data &g) {
  query_response_generator load_client_choice = [&g](int cid, sf::Packet query) -> handler_result {
    choice ch;

    return handler_switch(query >> ch, [&g, &ch, cid](handler_result &res) {
      g.apply_choice(ch, cid);
    });
  };

  // check if we are loading an autosave
  string filename = c.id + ".auto.save";
  // bool did_load = load_autosave(filename, c, g);
  bool did_load = false;

  g.rebuild_evm();
  g.pre_step();

  // all players know about other players' home solars
  if (!c.check_protocol(protocol::load_init, pack_game_handler(g, did_load))) return;

  while (true) {
    autosave_game(c, g);
    if (check_end(c, g)) return;

    if (!c.check_protocol(protocol::game_round, pack_game_handler(g, true))) {
      server::log("game_handler: failed check protocol: game_round!", "warning");
      return;
    }

    // idle the fleets and clear waypoints
    g.pre_step();

    // choices, expects: query + choice
    if (!c.check_protocol(protocol::choice, load_client_choice)) {
      server::log("game_handler: failed check protocol: choice!", "warning");
      return;
    }

    // simulation
    simulation_step(c, g);

    // cleanup
    g.end_step();
  }
}
