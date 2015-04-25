#include <algorithm>
#include <iostream>
#include <cmath>

#include "types.h"
#include "game_data.h"
#include "utility.h"
#include "research.h"

using namespace std;
using namespace st3;

idtype ship::id_counter = 0;
idtype solar::id_counter = 0;
idtype fleet::id_counter = 0;

point game_data::target_position(target_t t){
  string type = identifier::get_type(t);
  if (!type.compare(identifier::solar)){
    return solars[identifier::get_id(t)].position;
  }else if (!type.compare(identifier::fleet)){
    return fleets[identifier::get_id(t)].position;
  }else if (!type.compare(identifier::point)){
    return identifier::get_point(t);
  }else{
    cout << "target_position: " << t << ": unknown type!" << endl;
    exit(-1);
  }
}

idtype game_data::solar_at(point p){
  for (auto &x : solars){
    if (utility::l2d2(p - x.second.position) < pow(x.second.radius, 2)){
      return x.first;
    }
  }
}

void game_data::generate_fleet(point p, idtype owner, command &c, set<idtype> &sh){
  fleet f;
  idtype fid = fleet::id_counter++;
  f.com = c;
  f.position = p;
  f.radius = settings.fleet_default_radius;
  f.owner = owner;

  for (auto i : sh){
    ship &s = ships[i];
    s.fleet_id = fid;
    s.position = utility::random_point_polar(p, f.radius);
    s.angle = utility::point_angle(target_position(c.target) - p);
    s.owner = owner;
    s.was_killed = false;
    cout << "generated ship " << i << " at " << s.position.x << "x" << s.position.y << endl;
    f.ships.insert(i);
  }

  fleets[fid] = f;
}

void game_data::set_fleet_commands(idtype id, list<command> commands){
  fleet &s = fleets[id];
  idtype fid;

  // split fleets
  for (auto &x : commands){
    fid = fleet::id_counter++;
    fleet f;
    f.com = x;
    f.position = s.position;
    f.radius = s.radius;
    f.owner = s.owner;
    for (auto i : x.ships){
      if (s.ships.count(i)){
	ships[i].fleet_id = fid;
	s.ships.erase(i);
	f.ships.insert(i);
      }
    }

    fleets[fid] = f;
  }

  // check remove
  if (s.ships.empty()){
    fleets.erase(id);
  }
}

void game_data::set_solar_commands(idtype id, list<command> commands){
  solar::solar &s = solars[id];

  // create fleets
  set<idtype> ready;
  for (auto &x : commands){
    ready.clear();
    for (auto i : x.ships){
      if (s.ships.count(i)){
	ready.insert(i);
	s.ships.erase(i);
      }else{
	cout << "set_solar_commands: invalid ship id: " << i << endl;
	exit(-1);
      }
    }

    generate_fleet(s.position, s.owner, x, ready);
  }
}
 
void game_data::apply_choice(choice c, idtype id){
  cout << "game_data: running dummy apply choice" << endl;

  for (auto x : c.solar_choices){
    if (solars[x.first].owner != id){
      cout << "apply_choice: error: solar choice by player " << id << " for solar " << x.first << " owned by " << solars[x.first].owner << endl;
      exit(-1);
    }
    solar_choices[x.first] = x.second;
  }

  for (auto x : c.commands){
    if (!identifier::get_type(x.first).compare(identifier::solar)){
      // commands for solar
      idtype sid = identifier::get_id(x.first);

      // check ownership
      if (solars[sid].owner != id){
	cout << "apply_choice: invalid owner: " << solars[sid].owner << ", should be " << id << endl;
	exit(-1);
      }

      // apply the commands
      set_solar_commands(sid, x.second);
    }else if (!identifier::get_type(x.first).compare(identifier::fleet)){
      // command for fleet
      idtype fid = identifier::get_id(x.first);
      
      // check ownerships
      if (fleets[fid].owner != id){
	cout << "apply_choice: invalid owner: " << fleets[fid].owner << ", should be " << id << endl;
	exit(-1);
      }

      // apply the commands
      set_fleet_commands(fid, x.second);
    }else{
      cout << "apply_choice: invalid source: " << x.first << endl;
      exit(-1);
    }
  }
}

void game_data::allocate_grid(){
  ship_grid = new grid::tree();

  for (auto x : ships){
    ship_grid -> insert(x.first, x.second.position);
  }
}

void game_data::deallocate_grid(){
  delete ship_grid;
}

void game_data::increment(){
  list<idtype> fids;
  cout << "game_data: running increment" << endl;

  // update solar data
  for (auto &x : solars){
    solar_tick(x.first);
  }

  // buffer fleet ids as fleets may be removed in loop
  for (auto &x : fleets){
    fids.push_back(x.first);
  }

  for (auto fid : fids){
    fleet &f = fleets[fid];
    point to = target_position(f.com.target);

    // ship arithmetics
    set<idtype> sids(f.ships); // need to copy explicitly?
    for (auto i : sids){
      ship &s = ships[i];
      point delta;
      if (f.converge){
	delta = to - s.position;
      }else{
	delta = to - f.position;
      }
      s.angle = utility::point_angle(delta);
      s.position = s.position + utility::scale_point(utility::normv(s.angle), s.speed);
      ship_grid -> move(i, s.position);

      // check if ship s has reached a destination solar
      if (!identifier::get_type(f.com.target).compare(identifier::solar)){
	idtype sid = solar_at(s.position);
	if (sid == identifier::get_id(f.com.target)){
	  solar::solar &sol = solars[sid];
	  if (utility::l2d2(sol.position - s.position) < sol.radius * sol.radius){
	    if (sol.owner == s.owner){
	      ship_land(i, sid);
	    }else{
	      ship_bombard(i, sid);
	    }
	  }
	}
      }

      // ship interactions
      list<grid::iterator_type> buf = ship_grid -> search(s.position, s.interaction_radius);
      vector<grid::iterator_type> res(buf.begin(), buf.end());
      random_shuffle(res.begin(), res.end());
      for (auto k : res){
	if (ships[k.first].owner != s.owner){
	  if (ship_fire(i, k.first)) break;
	}
      }
    }

    // update fleet data
    if (!((f.update_counter++) % fleet::update_period)){
      int count;

      // position
      point p(0,0);
      count = 0;
      for (auto k : f.ships){
	ship &s = ships[k];
	p = p + s.position;
	if (++count > 20) break;
      }
      f.position = utility::scale_point(p, 1 / (float)count);

      // radius
      float r2 = 0;
      count = 0;
      for (auto k : f.ships){
	ship &s = ships[k];
	r2 = fmax(r2, utility::l2d2(s.position - f.position));
	if (++count > 20) break;
      }
      f.radius = sqrt(r2);

      // have arrived?
      point target = target_position(f.com.target);
      f.converge = utility::l2d2(target - f.position) < fleet::interact_d2;
      if (f.com.child_commands.size() && f.converge){
	// apply child commands - may destroy fleet f
	set_fleet_commands(fid, f.com.child_commands);
      }
    }
  }
}

// ****************************************
// SHIP INTERACTION
// ****************************************

void game_data::ship_land(idtype ship_id, idtype solar_id){
  idtype fid = ships[ship_id].fleet_id;

  // add to solar
  solars[solar_id].ships.insert(ship_id);

  // remove from fleet
  fleets[fid].ships.erase(ship_id);

  // remove fleet if empty
  if (fleets[fid].ships.empty()) fleets.erase(fid);

  // debug output
  cout << "landed ship " << ship_id << " on solar " << solar_id << endl;
}

void game_data::ship_bombard(idtype ship_id, idtype solar_id){
  solar::solar &sol = solars[solar_id];
  ship &s = ships[ship_id];

  sol.defense_current -= s.hp;

  cout << "ship " << ship_id << " bombards solar " << solar_id << ", resulting defense: " << sol.defense_current << endl;

  // todo: add some random destruction to solar

  if (sol.defense_current <= 0){
    sol.owner = s.owner;
    cout << "player " << sol.owner << " conquers solar " << solar_id << endl;
  }

  remove_ship(ship_id);
}

bool game_data::ship_fire(idtype s, idtype t){
  cout << "ship " << s << " fires on ship " << t << endl;
  if (--ships[t].hp < 1){
    cout << " -> ship " << t << " dies" << endl;
    remove_ship(t);
  }

  return true; // indicate ship initiative is over
}

void game_data::remove_ship(idtype i){
  ship &s = ships[i];

  // remove from fleet
  fleets[s.fleet_id].ships.erase(i);

  // remove from grid
  ship_grid -> remove(i);

  // remove from ships
  ships.erase(i);

  cout << "removed ship " << i << endl;
}

// produce a number of random solars with no owner according to
// settings
hm_t<idtype, solar::solar> game_data::random_solars(){
  hm_t<idtype, solar::solar> buf;
  int q_start = 20;

  for (int i = 0; i < settings.num_solars; i++){
    solar::solar s;
    s.radius = settings.solar_minrad + rand() % (int)(settings.solar_maxrad - settings.solar_minrad);
    s.position.x = s.radius + rand() % (int)(settings.width - 2 * s.radius);
    s.position.y = s.radius + rand() % (int)(settings.height - 2 * s.radius);
    s.owner = -1;
    s.defense_capacity = s.defense_current = rand() % q_start;
    buf[i] = s;
  }

  // shake the solars around till they don't overlap
  bool overlap = true;
  int margin = 10;

  cout << "game_data: shaking solars..." << endl;
  for (int n = 0; overlap && n < 100; n++){
    overlap = false;
    for (int i = 0; i < settings.num_solars; i++){
      for (int j = i + 1; j < settings.num_solars; j++){
	point delta = buf[i].position - buf[j].position;
	if (utility::l2norm(delta) < buf[i].radius + buf[j].radius + margin){
	  overlap = true;
	  buf[i].position = buf[i].position + utility::scale_point(delta, 0.1) + point(rand() % 4 - 2, rand() % 4 - 2);
	}
      }
    }
  }

  return buf;
}

// find the fairest selection of (player.id, start_solar.id) and store
// in start_solars, return unfairness estimate
float game_data::heuristic_homes(hm_t<idtype, solar::solar> solar_buf, hm_t<idtype, idtype> &start_solars){
  int ntest = 100;
  float umin = INFINITY;
  hm_t<idtype, idtype> test_solars;
  vector<idtype> solar_ids, player_ids;
  vector<bool> reach;
  int count = 0;
  
  if (players.size() > solar_buf.size()){
    cout << "heuristic homes: to many players: " << players.size() << " > " << solar_buf.size() << endl;
    exit(-1);
  }

  float range = settings.ship_speed * settings.frames_per_round;
  
  for (auto x : solar_buf) solar_ids.push_back(x.first);
  for (auto x : players) player_ids.push_back(x.first);
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
	solar::solar a = solar_buf[test_solars[player_ids[j]]];
	solar::solar b = solar_buf[test_solars[player_ids[k]]];
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

// players and settings should be set before build is called
void game_data::build(){
  if (players.empty()){
    cout << "game_data: build: no players!" << endl;
    exit(-1);
  }

  dt = 1;

  cout << "game_data: running dummy build" << endl;

  // build solars
  int ntest = 100;
  float unfairness = INFINITY;
  hm_t<idtype, idtype> test_homes;
  int q_start = 20;
  int d_start = 40;

  for (int i = 0; i < ntest && unfairness > 0; i++){
    hm_t<idtype, solar::solar> solar_buf = random_solars();
    float u = heuristic_homes(solar_buf, test_homes);
    if (u < unfairness){
      cout << "game_data::initialize: new best homes, u = " << u << endl;
      unfairness = u;
      solars = solar_buf;
      for (auto x : test_homes){
	solar::solar &s = solars[x.second];
	s.owner = x.first;
	s.defense_current = s.defense_capacity = d_start;
	// for (int j = 0; j < q_start; j++){
	//   ship::class_t key = utility::random_uniform() < 0.5 ? ship::class_scout : ship::class_fighter;
	//   research rbase;
	//   ship sh(key, rbase);
	//   s.add_ship(sh);
	// }
      }
    }
  }

  cout << "resulting solars:" << endl;
  for (auto x : solars){
    solar::solar s = x.second;
    cout << "solar " << x.first << ": p = " << s.position.x << "," << s.position.y << ": r = " << s.radius << ", quantity = " << s.ships.size() << endl;
  }
}

// solar
void st3::game_data::solar_tick(idtype id){
  int i;
  solar::solar &s = solars[id];
  solar::choice_t &c = solar_choices[id];
  research &r_base = players[s.owner].research_level;

  if (s.population_number <= 0){
    s.population_number = 0;
    return;
  }

  c.normalize();

  solar::solar buf(s);
  float Ph = s.population_number * s.population_happy;
  vector<float> P;

  cout << "solar " << id << " tick: " << endl;

  cout << "Population: ";
  // population assignments
  P.resize(solar::p_num);
  for (i = 0; i < solar::p_num; i++){
    P[i] = Ph * c.population[i];
    cout << P[i] << ", ";
  }
  cout << endl;

  cout << "Research: ";
  // research
  for (i = 0; i < research::r_num; i++){
    float allocated = P[solar::p_research] * c.dev.new_research[i];
    buf.dev.new_research[i] += fmin(allocated, s.dev.industry[solar::i_research]) * dt;
    cout << s.dev.new_research[i] << ", ";
  }
  cout << endl;

  // industry
  cout << "industry: ";
  for (i = 0; i < solar::i_num; i++){
    float allocated = P[solar::p_industry] * c.dev.industry[solar::i_infrastructure];
    buf.dev.industry[i] += fmin(allocated, s.dev.industry[solar::i_infrastructure]) * dt;
    cout << s.dev.industry[i] << ", ";
  }
  cout << endl;

  // fleet 
  // pre-check resource constraints?
  cout << "fleet: ";
  for (i = 0; i < solar::s_num; i++){
    vector<float> r = solar::development::ship_cost[i];
    for (auto &y : r) y /= dt;
    float allocated = P[solar::p_industry] * c.dev.industry[solar::i_ship] * c.dev.fleet_growth[i];
    float build = fmin(allocated, fmin(r_base.field[research::r_industry] * s.dev.industry[solar::i_ship], s.resource_constraint(r)));
    buf.dev.fleet_growth[i] += build * dt;
    for (int j = 0; j < solar::o_num; j++){
      buf.dev.resource[j] -= r[j] * dt;
    }
    cout << s.dev.fleet_growth[i] << ", ";
  }
  cout << endl;

  // resource
  cout << "resource: ";
  for (i = 0; i < solar::o_num; i++){
    float allocated = P[solar::p_resource] * c.dev.resource[i];
    float delta = fmin(allocated, s.dev.industry[solar::i_resource]) * dt;
    float r = fmin(delta, s.resource[i]);
    buf.resource[i] -= r * dt; // resources on solar
    buf.dev.resource[i] += r * dt; // resources in storage
    cout << s.dev.resource[i] << ", ";
  }
  cout << endl;

  // population
  buf.population_number += s.population_number * (r_base.field[research::r_population] + s.dev.industry[solar::i_agriculture] - s.population_number) * dt;
  buf.population_happy += (r_base.field[research::r_population] - P[solar::p_industry]) * dt;

  cout << "population: " << buf.population_number << "(" << buf.population_happy << ")" << endl;

  // assignment
  s.dev.new_research.swap(buf.dev.new_research);
  s.dev.industry.swap(buf.dev.industry);
  s.dev.resource.swap(buf.dev.resource);
  s.resource.swap(buf.resource);
  s.dev.fleet_growth.swap(buf.dev.fleet_growth);
  s.population_number = buf.population_number;
  s.population_happy = buf.population_happy;

  // new ships
  for (i = 0; i < solar::s_num; i++){
    while (s.dev.fleet_growth[i] >= 1){
      ship sh(i, r_base);
      idtype sid = ship::id_counter++;
      ships[sid] = sh;
      s.ships.insert(sid);
      s.dev.fleet_growth[i]--;
      cout << "ship " << sid << " was created for player " << s.owner << " at solar " << id << endl;
    }
  }
}


// find all dead ships and remove them
void game_data::cleanup(){
  hm_t<idtype, ship> buf;
  cout << "cleanup: start: " << ships.size() << " ships." << endl;
  for (auto x : ships){
    if (!x.second.was_killed) buf[x.first] = x.second;
  }
  ships.swap(buf);
  cout << "cleanup: end: " << ships.size() << " ships." << endl;
}
