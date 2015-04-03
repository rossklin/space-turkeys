#include <string>
#include <vector>
#include <queue>
#include <cmath>
#include <iostream>
#include <thread>

#include "protocol.h"
#include "game_data.h"
#include "com.h"
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
  int count = 0;
  
  p_confirm << protocol::confirm;

  while (true){
    // check end
    if (count++ > 2){
      packet.clear();
      packet << protocol::complete;
      c.check_protocol(protocol::game_round, packet);
      return;
    }

    // pre, expects: only query
    cout << "preload size: " << g.ships.size() << endl;
    packet.clear();
    packet << protocol::confirm;
    packet << g;
    c.check_protocol(protocol::game_round, packet);

    // choices, expects: query + choice
    c.check_protocol(protocol::choice, p_confirm);

    for (i = 0; i < c.clients.size(); i++){
      choice ch;
      if (c.clients[i].data >> ch){
	g.apply_choice(ch, c.clients[i].id);
      }else{
	cout << "choice for player " << c.clients[i].id << " failed to unpack!" << endl;
      }
    }

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
    g.cleanup();
    cout << "post cleanup size: " << g.ships.size() << endl;
  }
}

