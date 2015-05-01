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

using namespace std;
using namespace st3;
using namespace st3::server;

void st3::server::game_handler(com c, game_data g){
  sf::Packet packet, p_confirm;
  vector<sf::Packet> frames(g.settings.frames_per_round);
  int frame_count;
  unsigned int i;

  solar::development::initialize();
  
  p_confirm << protocol::confirm;

  while (true){
    g.allocate_grid();

    // check end
    int pid = -1;
    int psum = 0;
    for (auto &x : g.solars){
      if (x.second.owner > -1 && x.second.owner != pid){
	pid = x.second.owner;
	psum++;
      }
    }

    if (psum < 2){
      cout << "game complete" << endl;
      packet.clear();
      packet << protocol::complete;
      packet << g;
      c.check_protocol(protocol::game_round, packet);
      return;
    }

    // pre, expects: only query
    cout << "preload size: " << g.ships.size() << endl;
    packet.clear();
    packet << protocol::confirm;
    packet << g;
    c.check_protocol(protocol::game_round, packet);

    // idle the fleets
    g.pre_step();

    // choices, expects: query + choice
    c.check_protocol(protocol::choice, p_confirm);

    for (i = 0; i < c.clients.size(); i++){
      choice ch;
      if (*c.clients[i].data >> ch){
	g.apply_choice(ch, c.clients[i].id);
      }else{
	cout << "choice for player " << c.clients[i].id << " failed to unpack!" << endl;
      }
    }

    // remove non-updated waypoints
    g.post_choice_step();

    // simulation
    cout << "starting simulation ... " << endl;
    frame_count = 0;
    thread t(&com::distribute_frames, c, ref(frames), ref(frame_count));

    cout << "frames size: " << g.ships.size() << endl;
    for (frame_count = 0; frame_count < g.settings.frames_per_round; frame_count++){
      g.increment();
      frames[frame_count].clear();
      frames[frame_count] << protocol::confirm;
      if (!(frames[frame_count] << g)){
	cout << "failed to serialize frame" << endl;
	exit(-1);
      }
    }
    
    cout << "waiting for distribute_frames() ..." << endl;
    t.join();

    cout << "cleaning up game_data..." << endl;

    // cleanup
    g.end_step();
    g.deallocate_grid();
    cout << "post cleanup size: " << g.ships.size() << endl;
  }
}

