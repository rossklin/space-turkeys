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

bool game_data::target_position(target_t t, point &p){
  string type = identifier::get_type(t);
  idtype id = identifier::get_id(t);
  if ((!type.compare(identifier::solar)) && solars.count(id)){
    p = solars[id].position;
  }else if ((!type.compare(identifier::fleet)) && fleets.count(id)){
    p = fleets[identifier::get_id(t)].position;
  }else if ((!type.compare(identifier::waypoint)) && waypoints.count(identifier::get_string_id(t))){
    p = waypoints[identifier::get_string_id(t)].position;
  }else{
    cout << "target_position: " << t << ": not found!" << endl;
    return false;
  }

  return true;
}

idtype game_data::solar_at(point p){
  for (auto &x : solars){
    if (utility::l2d2(p - x.second.position) < pow(x.second.radius, 2)){
      return x.first;
    }
  }

  return -1;
}

void game_data::relocate_ships(command &c, set<idtype> &sh, idtype owner){
  fleet f;
  idtype fid = fleet::id_counter++;
  f.com = c;
  f.com.source = identifier::make(identifier::fleet, fid);
  f.owner = owner;
  f.update_counter = 0;

  // clear parent fleets
  for (auto i : sh){
    idtype sf = ships[i].fleet_id;
    fleets[sf].ships.erase(i);
    if (fleets[sf].ships.empty()){
      fleets.erase(sf);
      cout << "relocate ships: erase fleet " << sf << ", total " << fleets.size() << " fleets." << endl;
    }

    // set new fleet id
    ships[i].fleet_id = fid;
  }

  f.ships.insert(sh.begin(), sh.end());
  fleets[fid] = f;
  update_fleet_data(fid);

  cout << "relocate ships: added fleet " << fid << endl;
}

void game_data::generate_fleet(point p, idtype owner, command &c, set<idtype> &sh){
  if (sh.empty()) return;

  fleet f;
  idtype fid = fleet::id_counter++;
  f.com = c;
  f.com.source = identifier::make(identifier::fleet, fid);
  f.position = p;
  f.radius = settings.fleet_default_radius;
  f.owner = owner;

  for (auto i : sh){
    ship &s = ships[i];
    point buf;
    s.fleet_id = fid;
    s.position = utility::random_point_polar(p, f.radius);
    if (!target_position(c.target, buf)){
      cout << "generate fleet: invalid target: " << c.target << endl;
      exit(-1);
    }
    s.angle = utility::point_angle(buf - p);
    s.owner = owner;
    // s.was_killed = false;
    cout << "generated ship " << i << " at " << s.position.x << "x" << s.position.y << endl;
    f.ships.insert(i);
    ship_grid -> insert(i, s.position);
  }

  fleets[fid] = f;

  update_fleet_data(fid);
}

void game_data::set_fleet_commands(idtype id, list<command> commands){
  fleet &s = fleets[id];
  idtype fid;

  // split fleets
  for (auto &x : commands){
    fid = x.ships == s.ships ? id : fleet::id_counter++;
    fleet f;
    f.com = x;
    f.com.source = identifier::make(identifier::fleet, fid);
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

  // todo: security checks
  cout << "apply_choice for player " << id << ": inserting " << c.waypoints.size() << " waypoints:" << endl;

  // keep those waypoints which client sends
  waypoints.clear();
  for (auto &x : c.waypoints){
    if (identifier::get_waypoint_owner(x.first) != id){
      cout << "apply_choice: player " << id << " tried to insert waypoint owned by " << identifier::get_waypoint_owner(x.first) << endl;
      exit(-1);
    }

    waypoints[x.first] = x.second;
  }

  for (auto &x : c.solar_choices){
    if (solars[x.first].owner != id){
      cout << "apply_choice: error: solar choice by player " << id << " for solar " << x.first << " owned by " << solars[x.first].owner << endl;
      exit(-1);
    }
    solar_choices[x.first] = x.second;
  }

  for (auto x : c.commands){
    cout << "apply_choice: checking command key " << x.first << endl;

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
      cout << "apply_choice: invalid source: '" << x.first << "'" << endl;
      exit(-1);
    }
  }
}

void game_data::allocate_grid(){
  ship_grid = new grid::tree();

  for (auto &f : fleets){
    for (auto i : f.second.ships){
      ship_grid -> insert(i, ships[i].position);
    }
  }
}

void game_data::deallocate_grid(){
  delete ship_grid;
}

void game_data::update_fleet_data(idtype fid){
  fleet &f = fleets[fid];
    
  // update fleet data
  if (!((f.update_counter++) % fleet::update_period)){
    float speed = INFINITY;
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

    // radius, speed and vision
    float r2 = 0;
    f.vision = 0;
    for (auto k : f.ships){
      ship &s = ships[k];
      speed = fmin(speed, s.speed);
      r2 = fmax(r2, utility::l2d2(s.position - f.position));
      f.vision = fmax(f.vision, s.vision);
    }
    f.radius = fmax(sqrt(r2), fleet::min_radius);
    f.speed_limit = speed;

    // have arrived?
    if (!f.is_idle()){
      point target;
      if (target_position(f.com.target, target)){
	f.converge = utility::l2d2(target - f.position) < fleet::interact_d2;

	// set to idle and 'land' ships if converged to waypoint
	if (f.converge && !identifier::get_type(f.com.target).compare(identifier::waypoint)){
	  source_t wid = identifier::get_string_id(f.com.target);
	  f.com.target = identifier::make(identifier::idle, wid);
	  cout << "set fleet " << fid << " idle target: " << f.com.target << endl;
	}
      }else{
	cout << "fleet " << fid << ": target " << f.com.target << " missing, setting idle:0" << endl;
	f.com.target = identifier::target_idle;
      }
    }
  }
}

void game_data::increment(){
  list<idtype> fids;
  set<idtype> sids;
  cout << "game_data: running increment" << endl;

  // update solar data
  for (auto &x : solars){
    solar_tick(x.first);
  }

  // buffer fleet ids as fleets may be removed in loop
  for (auto &x : fleets){
    fids.push_back(x.first);
  }

  // move ships
  for (auto fid : fids){
    cout << "running increment for fleet " << fid << endl;
    fleet &f = fleets[fid];
    point to;
    if (!(target_position(f.com.target, to) || f.is_idle())){
      cout << "fleet " << fid << ": target " << f.com.target << " missing and not idle, setting idle:0" << endl;
      f.com.target = identifier::target_idle;
    }

    // ship arithmetics
    sids = f.ships; // need to copy explicitly?
    for (auto i : sids){
      ship &s = ships[i];

      // check fleet is not idle
      if (!f.is_idle()){
	point delta;
	if (f.converge){
	  delta = to - s.position;
	}else{
	  delta = to - f.position;
	}

	s.angle = utility::point_angle(delta);
	s.position = s.position + utility::scale_point(utility::normv(s.angle), f.speed_limit);
	ship_grid -> move(i, s.position);
      }
    }
  }


  for (auto fid : fids){
    if (fleets.count(fid)){
      fleet &f = fleets[fid];
      sids = f.ships;
      for (auto i : sids){
	ship &s = ships[i];
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

	if (ships.count(i)){
	  cout << "ship " << i << " interactions..." << endl;
	  // ship interactions
	  list<grid::iterator_type> res = ship_grid -> search(s.position, s.interaction_radius);
	  vector<grid::iterator_type> targ(res.begin(), res.end());
	  random_shuffle(targ.begin(), targ.end());
	  for (auto k : targ){
	    if (ships[k.first].owner != s.owner){
	      if (ship_fire(i, k.first)) break;
	    }
	  }
	}
      }
    }

    if (fleets.count(fid)) update_fleet_data(fid);
  }

  // remove killed ships
  auto it = ships.begin();
  while (it != ships.end()){
    if (it -> second.was_killed){
      remove_ship((it++) -> first);
    }else{
      it++;
    }
  }

  // waypoint triggers
  for (auto &x : waypoints){
    cout << "waypoint trigger: checking " << x.first << endl;
    waypoint &w = x.second;

    // trigger commands
    bool check;
    set<idtype> ready_ships, arrived_ships;
    list<command> remove;

    // compute landed ships
    arrived_ships.clear();
    for (auto &y : fleets){
      if (y.second.is_idle() && !identifier::get_string_id(y.second.com.target).compare(x.first)){
	arrived_ships += y.second.ships;
      }
    }
    
    remove.clear();
    for (auto &y : w.pending_commands){
      // check if all ships in command y are either landed or dead
      check = true;
      ready_ships.clear();
      for (auto i : y.ships){
	if (ships.count(i)){
	  if (arrived_ships.count(i)){
	    ready_ships.insert(i);
	  }else{
	    check = false;
	  }
	}
      }

      if (check){
	cout << "waypoint trigger: command targeting " << y.target << "ready with " << ready_ships.size() << " ships!" << endl;
	relocate_ships(y, ready_ships, identifier::get_waypoint_owner(x.first));
	remove.push_back(y);
      }else {
	cout << endl << "waypoint trigger: have ships: ";
	for (auto z : arrived_ships) cout << z << ",";
	cout << "need ships: ";
	for (auto z : y.ships) cout << z << ",";
	cout << endl;
      }
    }

    for (auto y : remove){
      w.pending_commands.remove(y);
    }
  }
}

// ****************************************
// SHIP INTERACTION
// ****************************************

void game_data::ship_land(idtype ship_id, idtype solar_id){
  idtype fid = ships[ship_id].fleet_id;

  // unset fleet id
  ships[ship_id].fleet_id = -1;

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
  float damage;

  switch(s.ship_class){
  case solar::s_scout:
  case solar::s_fighter:
    damage = s.hp * utility::random_uniform();
    break;
  case solar::s_bomber:
    damage = 1 + utility::random_uniform();
    break;
  }

  cout << "ship " << ship_id << " bombards solar " << solar_id << ", resulting defense: " << sol.defense_current << endl;

  // todo: add some random destruction to solar
  sol.defense_current -= damage;
  sol.population_number -= 10 * damage;
  sol.population_happy *= 0.9;

  if (sol.defense_current <= 0){
    sol.owner = s.owner;
    sol.population_number += 100; // todo: temporary fix
    cout << "player " << sol.owner << " conquers solar " << solar_id << endl;
  }

  remove_ship(ship_id);
}

bool game_data::ship_fire(idtype sid, idtype tid){
  float scout_accuracy = 0.3;
  float fighter_accuracy = 0.7;
  float fighter_rapidfire = 0.3;
  ship &s = ships[sid];
  ship &t = ships[tid];
  cout << "ship " << sid << " fires on ship " << tid << endl;

  switch(s.ship_class){
  case solar::s_scout:
    if (utility::random_uniform() < scout_accuracy){
      t.hp -= utility::random_uniform();
      cout << " -> hit!" << endl;
      if (t.hp <= 0){
	t.was_killed = true;
	cout << " -> ship " << tid << " dies!" << endl;
      }
    }
    return true;
  case solar::s_fighter:
    if (utility::random_uniform() < fighter_accuracy){
      t.hp -= utility::random_uniform();
      cout << " -> hit!" << endl;
      if (t.hp <= 0){
	t.was_killed = true;
	cout << " -> ship " << tid << " dies!" << endl;
      }
    }
    return utility::random_uniform() < fighter_rapidfire; // fighters may fire multiple times
  case solar::s_bomber:
    return true; // bombers dont fire at ships
  }

  return true; // indicate ship initiative is over
}

void game_data::remove_ship(idtype i){
  ship &s = ships[i];

  cout << "removing ship " << i << endl;

  // remove from fleet
  fleets[s.fleet_id].ships.erase(i);
  
  // check if fleet is empty
  if (fleets[s.fleet_id].ships.empty()){
    fleets.erase(s.fleet_id);
    cout << " --> fleet " << s.fleet_id << " is empty, removing" << endl;
  }

  // remove from grid
  ship_grid -> remove(i);

  // remove from ships
  ships.erase(i);
}

// produce a number of random solars with no owner according to
// settings
hm_t<idtype, solar::solar> game_data::random_solars(){
  hm_t<idtype, solar::solar> buf;
  int q_start = 10;

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

  dt = 0.1;

  cout << "game_data: running dummy build" << endl;

  // build solars
  int ntest = 100;
  float unfairness = INFINITY;
  hm_t<idtype, idtype> test_homes;
  int q_start = 10;
  int d_start = 20;

  for (int i = 0; i < ntest && unfairness > 0; i++){
    hm_t<idtype, solar::solar> solar_buf = random_solars();
    float u = heuristic_homes(solar_buf, test_homes);
    if (u < unfairness){
      cout << "game_data::initialize: new best homes, u = " << u << endl;
      unfairness = u;
      solars = solar_buf;
      ships.clear();
      for (auto x : test_homes){
	solar::solar &s = solars[x.second];
	s.owner = x.first;
	s.defense_current = s.defense_capacity = d_start;
	for (int j = 0; j < q_start; j++){
	  ship::class_t key = utility::random_uniform() < 0.5 ? solar::s_scout : solar::s_fighter;
	  research rbase;
	  ship sh(key, rbase);
	  idtype id = ship::id_counter++;
	  ships[id] = sh;
	  s.ships.insert(id);
	}
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

  if (s.owner < 0 || s.population_number <= 0){
    s.population_number = 0;
    return;
  }

  c.normalize();

  solar::solar buf(s);
  float Ph = s.population_number * s.population_happy;
  vector<float> P;

  // cout << "solar " << id << " tick: " << endl;

  // cout << "Population: ";
  // population assignments
  P.resize(solar::p_num);
  for (i = 0; i < solar::p_num; i++){
    P[i] = Ph * c.population[i];
    // cout << P[i] << ", ";
  }
  // cout << endl;

  // cout << "Research: ";
  // research
  for (i = 0; i < research::r_num; i++){
    float allocated = P[solar::p_research] * c.dev.new_research[i];
    buf.dev.new_research[i] += solar::research_per_person * fmin(allocated, s.dev.industry[solar::i_research]) * dt;
    // cout << s.dev.new_research[i] << ", ";
  }
  // cout << endl;

  // industry
  // cout << "industry: ";
  float i_sum = 0;
  float i_cap = 10000;
  for (i = 0; i < solar::i_num; i++){
    float allocated = P[solar::p_industry] * c.dev.industry[solar::i_infrastructure];
    float working_people = fmin(allocated, s.dev.industry[solar::i_infrastructure]);
    float build = solar::industry_per_person * working_people * r_base.field[research::r_industry];

    buf.dev.industry[i] += build * dt;
    i_sum += buf.dev.industry[i];
    // cout << s.dev.industry[i] << ", ";
  }

  if (i_sum > i_cap){
    for (auto &x : buf.dev.industry) x *= i_cap / i_sum;
  }
  // cout << endl;

  // fleet 
  // pre-check resource constraints?
  // cout << "fleet: ";
  for (i = 0; i < solar::s_num; i++){
    vector<float> r = solar::development::ship_cost[i];
    float allocated = P[solar::p_industry] * c.dev.industry[solar::i_ship] * c.dev.fleet_growth[i];
    float working_people = fmin(allocated, s.dev.industry[solar::i_ship]);
    float build = solar::fleet_per_person * r_base.field[research::r_industry] * working_people / solar::development::ship_buildtime[i];

    build = fmin(build, s.resource_constraint(r));

    buf.dev.fleet_growth[i] += build * dt;
    for (int j = 0; j < solar::o_num; j++){
      buf.dev.resource[j] -= r[j] * build * dt;
    }
    // cout << s.dev.fleet_growth[i] << ", ";
  }
  // cout << endl;
  // temp fix: can't have negative resources
  for (auto &x : buf.dev.resource) x = fmax(x, 0);

  // resource
  // cout << "resource: ";
  for (i = 0; i < solar::o_num; i++){
    float allocated = P[solar::p_resource] * c.dev.resource[i];
    float delta = solar::resource_per_person * fmin(allocated, s.dev.industry[solar::i_resource]);
    float r = fmin(delta, s.resource[i]);
    buf.resource[i] -= r * dt; // resources on solar
    buf.dev.resource[i] += r * dt; // resources in storage
    // cout << s.dev.resource[i] << ", ";
  }
  // cout << endl;

  // population
  float allocated = fmin(s.usable_area - i_sum, P[solar::p_agriculture]);
  float food_cap = (1 + 0.01 * utility::sigmoid(r_base.field[research::r_population] + s.dev.industry[solar::i_agriculture], solar::agriculture_boost_coefficient)) * allocated;
  float feed = solar::feed_boost_coefficient * (food_cap - s.population_number) / s.population_number;
  float birth_rate = solar::births_per_person + feed;
  float death_rate = solar::deaths_per_person / (r_base.field[research::r_population] + 1) + fmax(-feed, 0);

  cout << "population dynamics for " << id << " : " << endl;
  cout << "pop: " << s.population_number << ", alloc: " << allocated << ", cap: " << food_cap << ", feed: " << feed << ", birth: " << birth_rate << ", death: " << death_rate << endl;

  buf.population_number += (birth_rate - death_rate) * s.population_number * dt;
  buf.population_happy += (0.1 + 0.01 * utility::sigmoid(r_base.field[research::r_population] - c.population[solar::p_industry], 1000)) * dt;
  buf.population_happy = fmax(fmin(buf.population_happy, 1), 0);

  // cout << "population: " << buf.population_number << "(" << buf.population_happy << ")" << endl;

  // defense
  s.defense_current = fmin(s.defense_current + 0.01, s.defense_capacity);

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
      // cout << "ship " << sid << " was created for player " << s.owner << " at solar " << id << endl;
    }
  }
}

// clean up things that will be reloaded from client
void game_data::pre_step(){
  // idle all fleets
  for (auto &i : fleets){
    cout << "pre step: initialising fleet " << i.first << " to idle:0" << endl;
    fleets[i.first].com.target = identifier::target_idle;
  }
}

// pool research
void game_data::end_step(){
  cout << "end_step:" << endl;

  // pool research
  for (auto & i : solars){
    if (i.second.owner > -1){
      for (int j = 0; j < research::r_num; j++){
	players[i.second.owner].research_level.field[j] += i.second.dev.new_research[j];
	cout << i.first << " contributes " << i.second.dev.new_research[j] << " new research to player " << i.second.owner << " in field " << j << endl;
	i.second.dev.new_research[j] = 0;
      }
    }
  }
}

game_data game_data::limit_to(idtype id){
  // build limited game data object;
  game_data gc;

  cout << "game_data: limit to: " << id << endl;

  // load fleets
  for (auto &x : fleets){
    bool seen = x.second.owner == id;
    for (auto i = fleets.begin(); i != fleets.end() && !seen; i++){
      seen |= x.first != i -> first && i -> second.owner == id && utility::l2norm(x.second.position - i -> second.position) < i -> second.vision;
    }

    for (auto i = solars.begin(); i != solars.end() && !seen; i++){
      seen |= i -> second.owner == id && utility::l2norm(x.second.position - i -> second.position) < i -> second.vision;
    }
    if (seen) gc.fleets[x.first] = x.second;
  }

  // load ships
  for (auto &x : gc.fleets) {
    for (auto &y : x.second.ships){
      gc.ships[y] = ships[y];
    }
  }

  // load solars
  for (auto &x : solars){
    bool seen = x.second.owner == id;
    cout << "solar " << x.first << " at " << x.second.position << ": owned = " << seen << endl;
    for (auto i = fleets.begin(); i != fleets.end() && !seen; i++){
      seen |= i -> second.owner == id && utility::l2norm(x.second.position - i -> second.position) < i -> second.vision;
      if (seen){
	cout << "spotted by fleet " << i -> first << " at " << i -> second.position << endl;
      }
    }

    for (auto i = solars.begin(); i != solars.end() && !seen; i++){
      seen |= x.first != i -> first && i -> second.owner == id && utility::l2norm(x.second.position - i -> second.position) < i -> second.vision;
      if (seen){
	cout << "spotted by solar " << i -> first << " at " << i -> second.position << endl;
      }
    }
    if (seen) gc.solars[x.first] = x.second;
  }

  // load waypoints
  for (auto &x : waypoints){
    if (identifier::get_waypoint_owner(x.first) == id){
      gc.waypoints[x.first] = x.second;
    }
  }

  // load players and settings
  gc.players = players;
  gc.settings = settings;

  return gc;
}
