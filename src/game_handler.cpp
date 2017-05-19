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
  vector<entity_package> frames(g.settings.frames_per_round);
  int frame_count;

  cout << "starting simulation ... " << endl;
  frame_count = 0;
  thread t(&com::distribute_frames, c, ref(frames), ref(frame_count));

  for (frame_count = 0; frame_count < g.settings.frames_per_round; frame_count++){
    g.increment();
    frames[frame_count].copy_from(g);
  }

  cout << "waiting for distribute_frames() ..." << endl;
  t.join();

  for (auto f : frames) f.clear_entities();
}

void server::game_handler(com &c, game_data &g){
  sf::Packet packet, p_confirm;
  hm_t<sint, sf::Packet> packets;
  unsigned int i;

  auto check_end = [&c, &g] () -> bool{
    sf::Packet packet;
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
      packet.clear();
      packet << protocol::complete;
      packet << (psum == 1 ? string("The winner is: ") + c.clients[pid] -> name : string("The game is a tie"));
      c.check_protocol(protocol::game_round, packet);
      return true;
    }

    return false;
  };

  auto pack_g = [&g, &c, &packets] () {
    entity_package ep;
    g.rebuild_evm();
    ep.copy_from(g);
  
    // load_init, expects: only query
    for (auto x : c.clients) {
      entity_package buf;
      buf = ep;
      buf.limit_to(x.first);
    
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
  
  p_confirm << protocol::confirm;

  pack_g();
  if (!c.check_protocol(protocol::load_init, packets)) return;

  while (true){
    if (check_end()) return;

    pack_g();
    if (!c.check_protocol(protocol::game_round, packets)) return;

    // idle the fleets and clear waypoints
    g.pre_step();

    // choices, expects: query + choice
    if (!c.check_protocol(protocol::choice, p_confirm)) return;
    for (auto x : c.clients) load_client_choice(x.second, x.first);

    // simulation
    simulation_step(c, g);
    cout << "cleaning up game_data..." << endl;

    // cleanup
    g.end_step();
    cout << "post cleanup size: " << g.entity.size() << endl;
  }
}

