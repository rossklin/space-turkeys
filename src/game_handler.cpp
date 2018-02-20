#include <string>
#include <vector>
#include <queue>
#include <cmath>
#include <iostream>
#include <thread>
#include <fstream>

#include "protocol.h"
#include "game_data.h"
#include "com_server.h"
#include "serialization.h"
#include "game_handler.h"
#include "utility.h"
#include "server_handler.h"

using namespace std;
using namespace st3;
using namespace st3::server;

void simulation_step(com &c, game_data &g) {
  int n = g.settings.frames_per_round;
  vector<entity_package> frames(n);
  int frame_count = 0;

  query_handler handler = [&c, &frames, &frame_count] (int cid, sf::Packet q) -> handler_result {
    int idx;
    bool test = q >> idx;

    auto on_success = [&frames, &frame_count, idx, cid] (handler_result &result) {
      entity_package g;

      if (idx < 0) {
	// client done - leave default confirm status
      } else if (idx < frame_count) {
	// pack frame
	g = frames[idx];
	g.limit_to(cid);
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

  auto generate_frames = [&c, &g, &frames, &frame_count, n] () {
    handler::safely([&] () {
	for (frame_count = 0; frame_count < n; frame_count++){
	  g.increment();
	  frames[frame_count].copy_from(g);
	  if (c.thread_com != socket_t::tc_run) break;
	}
      }, [&] () {
	c.thread_com = socket_t::tc_stop;
      });
  };

  thread t(generate_frames);
  if (!c.check_protocol(protocol::frame, handler)) c.thread_com = socket_t::tc_stop;
  t.join();

  for (int i = 0; i < frame_count; i++) frames[i].clear_entities();
}

void server::game_handler(com &c, game_data &g){

  auto check_end = [&c, &g] () -> bool{
    if (c.thread_com != socket_t::tc_run) {
      server::log("game_handler::check_end: server shut down!");
      return true;
    }
    
    int pid = -1;
    int psum = 0;
    for (auto x : g.all<solar>()){
      if (x -> owner > -1 && x -> owner != pid){
	pid = x -> owner;
	psum++;
      }
    }

    if (psum < 2){
      query_handler h = [&c, psum, pid] (int cid, sf::Packet data) -> handler_result {
	handler_result res;
	string message;

	if (psum == 1) {
	  if (cid == pid) {
	    message = "You won the game!";
	  } else {
	    message = "Player " + to_string(pid) + ": " + c.clients[pid] -> name + " won the game.";
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
  };

  auto pack_g = [&g] (bool do_limit) -> query_handler {
    return [&g, do_limit] (int cid, sf::Packet query) -> handler_result {
      handler_result res;
      entity_package ep = g;

      if (do_limit) ep.limit_to(cid);
      res.response << protocol::confirm << ep;
      res.status = socket_t::tc_complete;

      return res;
    };
  };

  auto autosave = [&g, &c] () {
    if (c.gid.empty()) {
      throw classified_error("autosave: server com object missing gid!");
    }

    entity_package ep = g;
    sf::Packet p;

    if (!(p << ep)) {
      throw classified_error("autosave: failed to serialize!");
    }
    
    const void *data = p.getData();
    int n = p.getDataSize();

    string filename = c.gid + ".auto.save";
    ofstream of(filename, ios::binary);
    of.write((const char*)data, n);
    bool success = !of.fail();
    of.close();

    if (!success) {
      throw classified_error("autosave: failed to write to file!");
    }
  };

  query_handler load_client_choice = [&g] (int cid, sf::Packet query) -> handler_result {
    choice::choice ch;
    
    return handler_switch(query >> ch, [&g, &ch, cid] (handler_result &res) {
	g.apply_choice(ch, cid);
      });
  };

  // check if we are loading an autosave
  string filename = c.gid + ".auto.save";

  ifstream file(filename, ios::binary | ios::ate);

  if (file.good() && !g.settings.restart) {
    streamsize size = file.tellg();
    file.seekg(0, ios::beg);
    vector<char> buffer(size);
    if (file.read(buffer.data(), size)) {
      sf::Packet p;
      p.append((const void*)buffer.data(), size);
      
      entity_package ep;
      if (!(p >> ep)) {
	throw classified_error("autoload: failed to deserialize!");
      }

      // map players by name
      hm_t<int, client_t*> new_clients;
      for (auto x : g.players) {
	bool success = false;
	for (auto y : ep.players) {
	  if (x.second.name == y.second.name) {
	    if (new_clients.count(y.first)) {
	      throw classified_error("autoload: attempted to register multiple players on the same name!");
	    }
	    
	    new_clients[y.first] = c.clients[x.first];
	    new_clients[y.first] -> id = y.first;
	    server::log("autoload: mapped player " + y.second.name + " from id " + to_string(x.first) + " to id " + to_string(y.first));
	    success = true;
	  }
	}

	if (!success) {
	  throw classified_error("Failed to map id for player " + to_string(x.first));
	}
      }

      c.clients = new_clients;

      g.idc = ep.idc;
      g.settings = ep.settings;
      g.terrain = ep.terrain;
      g.discovered_universe = ep.discovered_universe;
      g.entity = ep.entity;
      g.players = ep.players;
    }
  }
  
  g.rehash_grid();
  g.rebuild_evm();

  // all players know about other players' home solars
  if (!c.check_protocol(protocol::load_init, pack_g(false))) return;

  while (true) {
    autosave();
    if (check_end()) return;
    
    if (!c.check_protocol(protocol::game_round, pack_g(true))) {
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

