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
  idtype sf;
  set<idtype> fleet_buf;
  idtype fid;

  // check if ships fill exactly one fleet
  for (auto i : sh) fleet_buf.insert(sf = ships[i].fleet_id);
  bool reassign = fleet_buf.size() == 1 && fleets[sf].ships == sh;

  if (reassign){
    fid = sf;
    fleets[sf].com = c;
    fleets[sf].com.source = identifier::make(identifier::fleet, fid);
  }else{
    fid = fleet::id_counter++;

    f.com = c;
    f.owner = owner;
    f.update_counter = 0;
    f.com.source = identifier::make(identifier::fleet, fid);

    // clear ships from parent fleets
    for (auto i : sh) fleets[ships[i].fleet_id].ships.erase(i);

    // remove empty parent fleets
    for (auto i : fleet_buf){
      if (fleets[i].ships.empty()){
	remove_fleet(i);
	cout << "relocate ships: erase fleet " << i << ", total " << fleets.size() << " fleets." << endl;
      }
    }

    // debug printout
    if (fleet_buf.size() == 1){
      cout << "relocate ship mismatch: " << endl << sf << ": ";
      for (auto i : fleets[sf].ships) cout << i << ",";
      cout << endl << "sh: ";
      for (auto i : sh) cout << i << ",";
      cout << endl;
    }

    // set new fleet id
    for (auto i : sh) ships[i].fleet_id = fid;

    f.ships = sh;
    fleets[fid] = f;
  }

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
    if (x.ships == s.ships){
      // maintain id for trackability
      s.com.target = x.target;
      // if all ships were assigned, break.
      break;
    }else{
      fid = fleet::id_counter++;
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
  }

  // check remove
  if (s.ships.empty()){
    remove_fleet(id);
  }
}

void game_data::set_solar_commands(idtype id, list<command> commands){
  solar::solar &s = solars[id];

  // create fleets
  for (auto &x : commands){
    for (auto i : x.ships){
      if (!s.ships.count(i)){
	cout << "set_solar_commands: invalid ship id: " << i << endl;
	exit(-1);
      }
    }

    s.ships -= x.ships;
    generate_fleet(s.position, s.owner, x, x.ships);
  }
}
 
void game_data::apply_choice(choice c, idtype id){
  cout << "game_data: running dummy apply choice" << endl;

  // todo: security checks
  cout << "apply_choice for player " << id << ": inserting " << c.waypoints.size() << " waypoints:" << endl;

  // keep those waypoints which client sends
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
  cout << "incr. begin fleet count: " << fleets.size() << endl;

  // update solar data
  for (auto &x : solars){
    solar_tick(x.first);
  }

  // buffer fleet ids as fleets may be removed in loop
  for (auto &x : fleets){
    fids.push_back(x.first);
  }

  cout << "pre move fleet count: " << fleets.size() << endl;

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

  cout << "pre ship interact fleet count: " << fleets.size() << endl;

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

  cout << "post ship interact fleet count: " << fleets.size() << endl;

  // remove killed ships
  auto it = ships.begin();
  while (it != ships.end()){
    if (it -> second.was_killed){
      remove_ship((it++) -> first);
    }else{
      it++;
    }
  }

  cout << "pre wp fleet count: " << fleets.size() << endl;

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
	cout << "post trig fleet count: " << fleets.size() << endl;
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

  cout << "incr. end fleet count: " << fleets.size() << endl;

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

  // remove from grid
  ship_grid -> remove(ship_id);

  // remove fleet if empty
  if (fleets[fid].ships.empty()) remove_fleet(fid);

  // debug output
  cout << "landed ship " << ship_id << " on solar " << solar_id << endl;
}

void game_data::ship_bombard(idtype ship_id, idtype solar_id){
  solar::solar &sol = solars[solar_id];
  ship &s = ships[ship_id];
  float damage;

  switch(s.ship_class){
  case solar::ship_scout:
  case solar::ship_fighter:
    damage = utility::random_uniform();
    break;
  case solar::ship_bomber:
    damage = 2 + 2 * utility::random_uniform();
    break;
  }

  cout << "ship " << ship_id << " bombards solar " << solar_id << ", resulting defense: " << sol.defense_current << endl;

  // todo: add some random destruction to solar
  sol.defense_current = fmax(sol.defense_current - damage, 0);
  sol.population_number = fmax(sol.population_number - 10 * damage, 0);
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
  case solar::ship_scout:
    if (utility::random_uniform() < scout_accuracy){
      t.hp -= utility::random_uniform();
      cout << " -> hit!" << endl;
      if (t.hp <= 0){
	t.was_killed = true;
	cout << " -> ship " << tid << " dies!" << endl;
      }
    }
    return true;
  case solar::ship_fighter:
    if (utility::random_uniform() < fighter_accuracy){
      t.hp -= utility::random_uniform();
      cout << " -> hit!" << endl;
      if (t.hp <= 0){
	t.was_killed = true;
	cout << " -> ship " << tid << " dies!" << endl;
      }
    }
    return utility::random_uniform() < fighter_rapidfire; // fighters may fire multiple times
  case solar::ship_bomber:
    return true; // bombers dont fire at ships
  }

  return true; // indicate ship initiative is over
}

void game_data::remove_fleet(idtype i){
  fleets.erase(i);
  remove_entities.push_back(identifier::make(identifier::fleet, i));
}

void game_data::remove_ship(idtype i){
  ship &s = ships[i];

  cout << "removing ship " << i << endl;

  // remove from fleet
  fleets[s.fleet_id].ships.erase(i);
  
  // check if fleet is empty
  if (fleets[s.fleet_id].ships.empty()){
    remove_fleet(s.fleet_id);
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

  cout << "game_data: running dummy build" << endl;

  // build solars
  int ntest = 100;
  float unfairness = INFINITY;
  hm_t<idtype, idtype> test_homes;
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
	s.resource = vector<float>(solar::resource_num, 1000);
	research rbase;

	ship shs(solar::ship_scout, rbase);
	idtype id = ship::id_counter++;
	ships[id] = shs;
	s.ships.insert(id);

	ship shf(solar::ship_fighter, rbase);
	id = ship::id_counter++;
	ships[id] = shf;
	s.ships.insert(id);
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
  float dt = settings.dt;
  float allocated, build;

  if (s.owner < 0 || s.population_number <= 0){
    s.population_number = 0;
    return;
  }

  c.normalize();

  solar::solar buf(s);
  float Ph = s.population_number * c.workers;
  vector<float> P;

  // cout << "solar " << id << " tick: " << endl;

  // cout << "Population: ";
  // capped population assignments
  P.resize(solar::work_num);
  for (i = 0; i < solar::work_num; i++){
    P[i] = fmin(Ph * c.sector[i], s.industry[i]);
    // cout << P[i] << ", ";
  }
  // cout << endl;

  // cout << "Research: ";
  // research
  for (i = 0; i < research::r_num; i++){
    allocated = P[solar::work_research] * c.subsector[solar::work_research][i];
    buf.new_research[i] += s.sub_increment(r_base, solar::work_research, i, allocated) * dt;
    // cout << s.new_research[i] << ", ";
  }
  // cout << endl;

  // industry
  // cout << "industry: ";
  float i_sum = 0;
  float i_cap = s.usable_area;
  for (i = 0; i < solar::work_num; i++){
    allocated = P[solar::work_expansion] * c.subsector[solar::work_expansion][i];
    build = s.sub_increment(r_base, solar::work_expansion, i, allocated);

    vector<float> r = st3::solar::industry_cost[i];
    build = fmin(build, s.resource_constraint(r));

    buf.industry[i] += build * dt;

    for (int j = 0; j < solar::resource_num; j++){
      buf.resource_storage[j] -= r[j] * build * dt;
    }

    i_sum += buf.industry[i];
    // cout << s.industry[i] << ", ";
  }

  if (i_sum > i_cap){
    for (auto &x : buf.industry) x *= i_cap / i_sum;
  }
  // cout << endl;

  // defense enhance
  allocated = P[solar::work_defense] * c.subsector[solar::work_defense][solar::defense_enhance];
  build = s.sub_increment(r_base, solar::work_defense, solar::defense_enhance, allocated);
  buf.defense_capacity += build * dt;  

  // defense repair
  allocated = P[solar::work_defense] * c.subsector[solar::work_defense][solar::defense_repair];
  build = s.sub_increment(r_base, solar::work_defense, solar::defense_repair, allocated);
  buf.defense_current = fmin(buf.defense_current + build, buf.defense_capacity);

  // fleet 
  // pre-check resource constraints?
  // cout << "fleet: ";
  for (i = 0; i < solar::ship_num; i++){
    vector<float> r = st3::solar::ship_cost[i];
    allocated = P[solar::work_ship] * c.subsector[solar::work_ship][i];
    build = s.sub_increment(r_base, solar::work_ship, i, allocated);
    build = fmin(build, s.resource_constraint(r));
    buf.fleet_growth[i] += build * dt;

    for (int j = 0; j < solar::resource_num; j++){
      buf.resource_storage[j] -= r[j] * build * dt;
    }
    // cout << s.fleet_growth[i] << ", ";
  }
  // cout << endl;
  // temp fix: can't have negative resources
  for (auto &x : buf.resource_storage) x = fmax(x, 0);

  // resource
  // cout << "resource: ";
  for (i = 0; i < solar::resource_num; i++){
    allocated = P[solar::work_resource] * c.subsector[solar::work_resource][i];
    float delta = s.sub_increment(r_base, solar::work_resource, i, allocated);
    float r = fmin(delta, s.resource[i]);
    buf.resource[i] -= r * dt; // resources on solar
    buf.resource_storage[i] += r * dt; // resources in storage
    // cout << s.resource_storage[i] << ", ";
  }
  // cout << endl;

  // population
  allocated = fmin(s.usable_area - i_sum, s.population_number * s.population_happy * (1 - c.workers));

  cout << "population dynamics for " << id << " : " << endl;

  buf.population_number += s.pop_increment(r_base, allocated) * dt;
  buf.population_happy += 0.001 * (1 + 0.1 * utility::sigmoid(r_base.field[research::r_population] - P[solar::work_ship], 1000)) * dt;
  buf.population_happy = fmax(fmin(buf.population_happy, 1), 0);

  // cout << "population: " << buf.population_number << "(" << buf.population_happy << ")" << endl;

  // assignment
  s.industry.swap(buf.industry);
  s.new_research.swap(buf.new_research);
  s.resource_storage.swap(buf.resource_storage);
  s.resource.swap(buf.resource);
  s.fleet_growth.swap(buf.fleet_growth);
  s.population_number = buf.population_number;
  s.population_happy = buf.population_happy;
  s.defense_current = buf.defense_current;
  s.defense_capacity = buf.defense_capacity;

  // new ships
  for (i = 0; i < solar::ship_num; i++){
    while (s.fleet_growth[i] >= 1){
      ship sh(i, r_base);
      idtype sid = ship::id_counter++;
      ships[sid] = sh;
      s.ships.insert(sid);
      s.fleet_growth[i]--;
      // cout << "ship " << sid << " was created for player " << s.owner << " at solar " << id << endl;
    }
  }
}

// clean up things that will be reloaded from client
void game_data::pre_step(){
  // idle all non-idle fleets
  for (auto &i : fleets){
    cout << "pre step: initialising fleet " << i.first << " to idle:0" << endl;
    if (!i.second.is_idle()){
      fleets[i.first].com.target = identifier::target_idle;
    }
  }

  // clear waypoints
  waypoints.clear();
}

// pool research and remove unused waypoints
void game_data::end_step(){
  cout << "end_step:" << endl;

  bool check;
  list<source_t> remove;

  for (auto & i : waypoints){
    check = false;
    
    // check for fleets targeting this waypoint
    for (auto &j : fleets){
      check |= !identifier::get_string_id(j.second.com.target).compare(i.first);
    }

    // check for waypoints with commands targeting this waypoint
    for (auto &j : waypoints){
      for (auto &k : j.second.pending_commands){
	check |= !identifier::get_string_id(k.target).compare(i.first);
      }
    }

    if (!check) remove.push_back(i.first);
  }

  for (auto & i : remove) {
    waypoints.erase(i);
    remove_entities.push_back(identifier::make(identifier::waypoint, i));
    cout << "end_step: removed waypoint " << i << endl;
  }

  // pool research
  for (auto & i : solars){
    if (i.second.owner > -1){
      for (int j = 0; j < research::r_num; j++){
	players[i.second.owner].research_level.field[j] += i.second.new_research[j];
	cout << i.first << " contributes " << i.second.new_research[j] << " new research to player " << i.second.owner << " in field " << j << endl;
	i.second.new_research[j] = 0;
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

  // load solars
  for (auto &x : solars){
    bool seen = x.second.owner == id;
    for (auto i = fleets.begin(); i != fleets.end() && !seen; i++){
      seen |= i -> second.owner == id && utility::l2norm(x.second.position - i -> second.position) < i -> second.vision;
    }

    for (auto i = solars.begin(); i != solars.end() && !seen; i++){
      seen |= x.first != i -> first && i -> second.owner == id && utility::l2norm(x.second.position - i -> second.position) < i -> second.vision;
    }
    if (seen) gc.solars[x.first] = x.second;
  }

  // load ships
  for (auto &x : gc.fleets) {
    for (auto &y : x.second.ships){
      gc.ships[y] = ships[y];
    }
  }

  for (auto &x : gc.solars) {
    for (auto &y : x.second.ships){
      gc.ships[y] = ships[y];
    }
  }

  // load waypoints
  for (auto &x : waypoints){
    if (identifier::get_waypoint_owner(x.first) == id){
      cout << "waypoint " << x.first << " included" << endl;
      gc.waypoints[x.first] = x.second;
    }else{
      cout << "waypoint " << x.first << " excluded" << endl;
    }
  }

  // load players and settings
  gc.players = players;
  gc.settings = settings;
  gc.remove_entities = remove_entities;

  return gc;
}
