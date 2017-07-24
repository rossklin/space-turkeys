#include <string>
#include <vector>
#include <queue>
#include <cmath>
#include <iostream>
#include <thread>

#include "protocol.h"
#include "game_data.h"
#include "com_server.h"
#include "serialization.h"
#include "game_handler.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace st3::server;

void simulation_step(com &c, game_data &g) {
  int n = g.settings.frames_per_round;
  vector<entity_package> frames(n);
  int frame_count = 0;

  query_handler handler = [&c, &frames, &frame_count] (int cid, sf::Packet q) -> handler_result {
    handler_result result;
    protocol_t input;
    int idx;
    entity_package g;

    // default to invalid status
    result.status = socket_t::tc_stop;
    result.response << protocol::invalid;

    if (q >> input) {
      if (input == protocol::frame) {
	if (q >> idx) {
	  if (idx < 0) {
	    // client done
	    result.response.clear();
	    result.response << protocol::confirm;
	    result.status = socket_t::tc_complete;
	  } else if (idx < frame_count) {
	    // pack frame
	    g = frames[idx];
	    g.limit_to(cid);
	    result.response.clear();
	    result.response << protocol::confirm << g;
	    result.status = socket_t::tc_run;
	  } else {
	    // invalid idx
	  }
	} else {
	  // no idx supplied
	}
      } else {
	// unexpected query
      }
    } else {
      // no query supplied
    }

    return result;
  };

  auto generate_frames = [&c, &g, &frames, &frame_count, n] () {
    try {
      for (frame_count = 0; frame_count < n; frame_count++){
	g.increment();
	frames[frame_count].copy_from(g);
	if (c.thread_com != socket_t::tc_run) break;
      }
    } catch (exception e) {
      c.thread_com = socket_t::tc_stop;
    }
  };

  thread t(generate_frames);
  if (!c.check_protocol(handler)) c.thread_com = socket_t::tc_stop;
  t.join();

  for (int i = 0; i < frame_count; i++) frames[i].clear_entities();
}

void server::game_handler(com &c, game_data &g){
  hm_t<sint, sf::Packet> packets;

  auto check_end = [&c, &g, &packets] () -> bool{
    if (c.thread_com != socket_t::tc_run) return true;
    
    int pid = -1;
    int psum = 0;
    for (auto x : g.all<solar>()){
      if (x -> owner > -1 && x -> owner != pid){
	pid = x -> owner;
	psum++;
      }
    }

    if (psum < 2){
      cout << "game complete" << endl;
      for (auto x : c.clients) {
	string message;

	if (psum == 1) {
	  if (x.first == pid) {
	    message = "You won the game!";
	  } else {
	    message = "Player " + to_string(pid) + ": " + x.second -> name + " won the game.";
	  }
	} else {
	  message = "The game is a tie.";
	}
	
	packets[x.first].clear();
	packets[x.first] << protocol::complete;
	packets[x.first] << message;
      }
      
      c.check_protocol(com::basic_query_handler(protocol::game_round, packets));
      return true;
    }

    return false;
  };

  auto pack_g = [&g, &c, &packets] (bool do_limit) {
    entity_package ep;
    g.rebuild_evm();
    ep.copy_from(g);
  
    // load_init, expects: only query
    for (auto x : c.clients) {
      entity_package buf;
      buf = ep;
      if (do_limit) buf.limit_to(x.first);
    
      packets[x.first].clear();
      packets[x.first] << protocol::confirm;
      packets[x.first] << buf;
    }

    ep.clear_entities();
  };

  auto load_client_choice = [&g] (client_t *client, idtype id) {
    choice::choice ch;
    if (client -> data >> ch){
      g.apply_choice(ch, id);
    }else{
      cout << "choice for player " << client -> name << " failed to unpack!" << endl;
    }
  };

  pack_g(false);
  if (!c.check_protocol(com::basic_query_handler(protocol::load_init, packets))) return;

  while (true) {
    if (check_end()) return;

    pack_g(true);
    if (!c.check_protocol(com::basic_query_handler(protocol::game_round, packets))) return;

    // idle the fleets and clear waypoints
    g.pre_step();

    // choices, expects: query + choice
    if (!c.check_protocol(com::basic_query_handler(protocol::choice, protocol::confirm))) return;
    for (auto x : c.clients) load_client_choice(x.second, x.first);

    // simulation
    simulation_step(c, g);
    cout << "cleaning up game_data..." << endl;

    // cleanup
    g.end_step();
    cout << "post cleanup size: " << g.entity.size() << endl;
  }
}

