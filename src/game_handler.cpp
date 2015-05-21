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
  vector<game_data> frames(g.settings.frames_per_round);
  vector<sf::Packet> packets(c.clients.size());
  int frame_count;
  unsigned int i;

  st3::solar::initialize();
  
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
      packet << (psum == 1 ? c.clients[pid].name : string("tie"));
      c.check_protocol(protocol::game_round, packet);
      return;
    }

    // pre, expects: only query
    cout << "preload size: " << g.ships.size() << endl;
    for (int i = 0; i < c.clients.size(); i++){
      packets[i].clear();
      packets[i] << protocol::confirm;
      packets[i] << g.limit_to(c.clients[i].id);
    }
    c.check_protocol(protocol::game_round, packets);

    // idle the fleets and clear waypoints
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

    // simulation
    cout << "starting simulation ... " << endl;
    frame_count = 0;
    thread t(&com::distribute_frames, c, ref(frames), ref(frame_count));

    for (frame_count = 0; frame_count < g.settings.frames_per_round; frame_count++){
      g.increment();
      frames[frame_count] = g;
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

