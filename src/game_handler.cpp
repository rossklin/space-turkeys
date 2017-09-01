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

  query_handler load_client_choice = [&g] (int cid, sf::Packet query) -> handler_result {
    choice::choice ch;
    
    return handler_switch(query >> ch, [&g, &ch, cid] (handler_result &res) {
	g.apply_choice(ch, cid);
      });
  };

  g.rebuild_evm();
  if (!c.check_protocol(protocol::load_init, pack_g(false))) return;

  while (true) {
    if (check_end()) return;
    
    if (!c.check_protocol(protocol::game_round, pack_g(true))) return;

    // idle the fleets and clear waypoints
    g.pre_step();

    // choices, expects: query + choice
    if (!c.check_protocol(protocol::choice, load_client_choice)) return;

    // simulation
    simulation_step(c, g);
    cout << "cleaning up game_data..." << endl;

    // cleanup
    g.end_step();
    cout << "post cleanup size: " << g.entity.size() << endl;
  }
}

