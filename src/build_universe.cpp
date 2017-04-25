#include <iostream>

#include "build_universe.h"
#include "utility.h"
#include "cost.h"

using namespace std;
using namespace st3;

// produce a number of random solars with no owner according to
// settings
hm_t<combid, solar> build_universe::random_solars(game_settings settings){
  hm_t<combid, solar> buf;
  int q_start = 10;
  vector<combid> idbuf(settings.num_solars);

  for (int i = 0; i < settings.num_solars; i++){
    solar s;
    s.id = identifier::make(solar::class_id, i);
    idbuf[i] = s.id;
    
    s.radius = settings.solar_minrad + rand() % (int)(settings.solar_maxrad - settings.solar_minrad);
    s.position.x = s.radius + rand() % (int)(settings.width - 2 * s.radius);
    s.position.y = s.radius + rand() % (int)(settings.height - 2 * s.radius);
    s.owner = -1;

    s.population = 0;
    s.happiness = 1;
    s.research = 0;
    
    s.water = 1000 * utility::random_uniform();
    s.space = 1000 * utility::random_uniform();
    s.ecology = utility::random_uniform();

    for (auto v : cost::keywords::resource)
      s.resource[v].available = 1000 * utility::random_uniform();

    buf[s.id] = s;
  }

  // shake the solars around till they don't overlap
  float margin = 10;

  cout << "game_data: shaking solars..." << endl;
  for (int n = 0; n < 1000; n++){
    combid i = idbuf[utility::random_int(idbuf.size())];
    combid j = idbuf[utility::random_int(idbuf.size())];

    // degenerate case i == j has no effect
    point delta = buf[i].position - buf[j].position;
    if (utility::l2norm(delta) < buf[i].radius + buf[j].radius + margin){
      buf[i].position = buf[i].position + utility::scale_point(delta, 0.1) + point(utility::random_normal(), utility::random_normal());
    }
  }

  return buf;
}

// find the fairest selection of (player.id, start_solar.id) and store
// in start_solars, return unfairness estimate
float build_universe::heuristic_homes(hm_t<combid, solar> solar_buf, hm_t<idtype, combid> &start_solars, game_settings settings, vector<idtype> player_ids){
  int ntest = 100;
  float umin = INFINITY;
  hm_t<idtype, combid> test_solars;
  vector<combid> solar_ids;
  vector<bool> reach;
  int count = 0;
  
  if (player_ids.size() > solar_buf.size()){
    throw runtime_error("heuristic homes: to many players");
  }

  float range = settings.ship_speed * settings.frames_per_round;
  
  for (auto x : solar_buf) solar_ids.push_back(x.first);
  reach.resize(player_ids.size());

  for (int i = 0; i < ntest; i++){
    // random test setup
    for (auto j : player_ids){
      test_solars[j] = solar_ids[rand() % solar_ids.size()];
    }

    // compute number of owned solars in reach of at least one other
    // owned solar
    
    for (int j = 0; j < player_ids.size(); j++){
      reach[j] = false;
    }

    count = 0;
    for (int j = 0; j < player_ids.size(); j++){
      for (int k = j + 1; k < player_ids.size(); k++){
	solar a = solar_buf[test_solars[player_ids[j]]];
	solar b = solar_buf[test_solars[player_ids[k]]];
	bool r = pow(a.position.x - b.position.x, 2) + pow(a.position.y - b.position.y, 2) < range;
	reach[j] = reach[j] || r;
	reach[k] = reach[k] || r;
      }
      count += reach[j];
    }

    if (count < umin){
      cout << "heuristic homes: new best reach count: " << count << endl;
      umin = count;
      start_solars = test_solars;
    }
  }

  return umin;
}
