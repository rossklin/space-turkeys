#include <algorithm>
#include <iostream>
#include <cmath>

#include "types.h"
#include "game_data.h"
#include "utility.h"
#include "research.h"

using namespace std;
using namespace st3;
using namespace cost;

idtype ship::id_counter = 0;
idtype solar::id_counter = 0;
idtype fleet::id_counter = 0;
idtype waypoint::id_counter = 0;

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
    cout << "relocate: reassign" << endl;
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
  // evaluate commands in random order
  vector<command> buf(commands.begin(), commands.end());
  random_shuffle(buf.begin(), buf.end());
  for (auto &x : buf){
    if (x.ships == s.ships){
      // maintain id for trackability
      s.com.target = x.target;
      s.com.action = x.action;
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
 
void game_data::apply_choice(choice::choice c, idtype id){
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

    if (identifier::get_type(x.first) == identifier::solar){
      // commands for solar
      idtype sid = identifier::get_id(x.first);

      // check ownership
      if (solars[sid].owner != id){
	cout << "apply_choice: invalid owner: " << solars[sid].owner << ", should be " << id << endl;
	exit(-1);
      }

      // apply the commands
      set_solar_commands(sid, x.second);
    }else if (identifier::get_type(x.first) == identifier::fleet){
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

  // need to update fleet data?
  if (((f.update_counter++) % fleet::update_period)) return;
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
      bool reach = utility::l2d2(target - f.position) < fleet::interact_d2;

      // set fleet converge status for appropriate actions
      if (f.com.action == command::action_land
	  || f.com.action == command::action_attack
	  || f.com.action == command::action_colonize) {
	f.converge = reach;
      }

      // set to idle and 'land' ships if converged to waypoint
      if (reach && identifier::get_type(f.com.target) == identifier::waypoint && f.com.action == command::action_waypoint){
	source_t wid = identifier::get_string_id(f.com.target);
	f.com.target = identifier::make(identifier::idle, wid);
	cout << "set fleet " << fid << " idle target: " << f.com.target << endl;
      }else if (reach && identifier::get_type(f.com.target) == identifier::waypoint){
	cout << "fleet " << fid << " converged to waypoint but invalid action: " << f.com.action << endl;
	exit(-1);
      }

      // check fleet joins
      if (f.com.action == command::action_join && reach){
	if (identifier::get_type(f.com.target) != identifier::fleet){
	  cout << "fleet " << fid << " trying to join non-fleet " << f.com.target << endl;
	  exit(-1);
	}

	idtype target_id = identifier::get_id(f.com.target);
	fleet &fleet_t = fleets[target_id];
	for (auto i : f.ships){
	  ships[i].fleet_id = target_id;
	  fleet_t.ships.insert(i);
	}
	remove_fleet(fid);
	return;
      }
    }else{
      cout << "fleet " << fid << ": target " << f.com.target << " missing, setting idle:0" << endl;
      f.com.target = identifier::target_idle;
    }
  }

  // target left sight?
  if ((!f.is_idle())
      && (identifier::get_type(f.com.target) == identifier::fleet)
      && (!fleet_seen_by(identifier::get_id(f.com.target), f.owner))){

    cout << "fleet " << fid << " looses sight of " << f.com.target << endl;
    // create a waypoint and reset target
    waypoint w;
    if (!target_position(f.com.target, w.position)){
      cout << "unset fleet target: fleet position not found: " << f.com.target << endl;
      exit(-1);
    }

    string wid = to_string(f.owner) + "#" + to_string(--waypoint::id_counter);
    waypoints[wid] = w;
    f.com.target = identifier::make(identifier::waypoint, wid);
    f.com.action = command::action_waypoint;
  }
}

void game_data::increment(){
  list<idtype> fids;
  set<idtype> sids;
  cout << "incr. begin fleet count: " << fleets.size() << endl;

  // update solar data
  for (auto &x : solars) solar_tick(x.first);

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

      // load weapons
      s.load = fmin(s.load + 1, s.load_time);

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
	if (identifier::get_type(f.com.target) == identifier::solar){
	  idtype sid = solar_at(s.position);
	  if (sid == identifier::get_id(f.com.target)){
	    solar::solar &sol = solars[sid];
	    if (utility::l2d2(sol.position - s.position) < sol.radius * sol.radius){
	      // ship interacts with solar
	      if (f.com.action == command::action_land){
		ship_land(i, sid);
	      }else if (f.com.action == command::action_attack){
		ship_bombard(i, sid);
	      }else if (f.com.action == command::action_colonize){
		// non-colonizer ships will be sent to ship_bombard
		ship_colonize(i, sid);
	      }
	    }
	  }
	}

	// ship interactions
	if (ships.count(i) && s.load >= s.load_time && s.damage_ship > 0){
	  cout << "ship " << i << " interactions..." << endl;
	  s.load = 0;

	  // find targetable ships
	  list<grid::iterator_type> res = ship_grid -> search(s.position, s.interaction_radius);
	  list<idtype> buf;
	  for (auto &x : res){
	    if (ships[x.first].owner != s.owner) buf.push_back(x.first);
	  }

	  // fire at a random enemy
	  if (!buf.empty()){
	    vector<idtype> targ(buf.begin(), buf.end());
	    idtype k = targ[rand() % targ.size()];
	    ship_fire(i, k); 
	  }
	}
      }
    }

    if (fleets.count(fid)) update_fleet_data(fid);
  }

  cout << "post ship interact fleet count: " << fleets.size() << endl;

  // solar turrets fire
  for (auto &s : solars){
    for (auto &t : s.second.turrets){
      t.load = fmin(t.load + 1, t.load_time);
      if (t.damage > 0 && t.load >= t.load_time){

	// find targetable ships
	list<grid::iterator_type> res = ship_grid -> search(s.second.position, t.range);
	list<idtype> buf;
	for (auto &x : res){
	  if (ships[x.first].owner != s.second.owner) buf.push_back(x.first);
	}

	// fire at a random enemy
	if (!buf.empty()){
	  t.load = 0;
	  vector<idtype> targ(buf.begin(), buf.end());
	
	  int tid = targ[rand() % targ.size()];
	  cout << "turret from solar " << s.first << " fires at ship " << tid << endl;
	  if (utility::random_uniform() < t.accuracy){
	    ships[tid].hp -= utility::random_uniform(0, t.damage);
	    if (ships[tid].hp <= 0) ships[tid].was_killed = true;
	    cout << " -> hit" << endl;
	  }else{
	    cout << " -> miss" << endl;
	  }
	}else{
	  cout << "solar " << s.first << ": turret: no enemies!" << endl;
	}
      }
    }
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

    // evaluate commands in random order
    vector<command> buf(w.pending_commands.begin(), w.pending_commands.end());
    random_shuffle(buf.begin(), buf.end());
    cout << "relocate from " << x.first << ": command order: " << endl;
    for (auto &y : buf){
      cout << " -> " << y.target << endl;
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

void game_data::ship_colonize(idtype ship_id, idtype solar_id){
  solar::solar &sol = solars[solar_id];
  ship &s = ships[ship_id];

  // check if solar already colonized
  if (sol.owner == s.owner){
    ship_land(ship_id, solar_id);
    return;
  }

  // check colonization
  if (s.ship_class == cost::keywords::key_colonizer){
    if (sol.owner == -1 && !sol.has_defense()){
      sol.colonization_attempts[s.owner] = ship_id;
    }
  }else{
    ship_bombard(ship_id, solar_id);
  }
}

void game_data::ship_bombard(idtype ship_id, idtype solar_id){
  solar::solar &sol = solars[solar_id];
  ship &s = ships[ship_id];
  float damage;

  // check if solar already captured
  if (sol.owner == s.owner){
    ship_land(ship_id, solar_id);
    return;
  }

  // check if defenses already destroyed
  if (sol.owner == -1 && !sol.has_defense()){
    cout << "ship_bombard: neutral: no defense!" << endl;
    return;
  }

  // deal damage
  if (s.load >= s.load_time && utility::random_uniform() < s.accuracy){
    s.load = 0;
    sol.damage_taken[s.owner] += utility::random_uniform(0, s.damage_solar);
  }

  cout << "ship " << ship_id << " bombards solar " << solar_id << ", damage: " << damage << endl;
}

void game_data::solar_effects(int solar_id){
  solar::solar &sol = solars[solar_id];
  float total_damage = 0;
  float highest_id = -1;
  float highest_sum = 0;

  // analyse damage
  for (auto x : sol.damage_taken) {
    total_damage += x.second;
    if (x.second > highest_sum){
      highest_sum = x.second;
      highest_id = x.first;
    }

    cout << "solar_effects: " << x.second << " damage from player " << x.first << endl;
  }

  if (highest_id > -1){
    // todo: add some random destruction to solar
    sol.damage_turrets(total_damage);
    sol.population = fmax(sol.population - 10 * total_damage, 0);
    sol.happiness *= 0.9;

    if (sol.owner > -1 && !sol.has_defense()){
      sol.owner = highest_id;
      cout << "player " << sol.owner << " conquers solar " << solar_id << endl;
    }else{
      cout << "resulting defense for solar " << solar_id << ": " << sol.has_defense() << endl;
    }
  }

  // colonization: randomize among attempts
  float count = 0;
  float num = sol.colonization_attempts.size();
  for (auto i : sol.colonization_attempts){
    if (utility::random_uniform() <= 1 / (num - count++)){
      players[i.first].research_level.colonize(&sol);
      sol.owner = i.first;
      cout << "player " << sol.owner << " colonizes solar " << solar_id << endl;
      remove_ship(i.second);
    }
  }
}

void game_data::ship_fire(idtype sid, idtype tid){
  ship &s = ships[sid];
  ship &t = ships[tid];
  cout << "ship " << sid << " fires on ship " << tid << endl;

  if (s.damage_ship > 0 && utility::random_uniform() < s.accuracy){
    t.hp -= utility::random_uniform(0, s.damage_ship);
    cout << " -> hit!" << endl;
    if (t.hp <= 0){
      t.was_killed = true;
      cout << " -> ship " << tid << " dies!" << endl;
    }
  }
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

    s.population = 0;
    s.happiness = 1;
    s.research = 0;
    
    s.water = 1000 * utility::random_uniform();
    s.space = 1000 * utility::random_uniform();
    s.ecology = utility::random_uniform();

    for (auto v : keywords::resource)
      s.resource[v].available = 1000 * utility::random_uniform();

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
  static research::data rbase;
  
  if (players.empty()){
    cout << "game_data: build: no players!" << endl;
    exit(-1);
  }

  cout << "game_data: running dummy build" << endl;

  resource_allocation<resource_data> initial_resources;
  for (auto v : keywords::resource)
    initial_resources[v].available = 1000;
  
  // build solars
  int ntest = 100;
  float unfairness = INFINITY;
  hm_t<idtype, idtype> test_homes;
  int d_start = 10;

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
	s.resource = initial_resources;
	s.water = 1000;
	s.space = 1000;
	s.population = 100;
	s.happiness = 1;

	idtype id = ship::id_counter++;
	ships[id] = rbase.build_ship("scout");
	s.ships.insert(id);

	id = ship::id_counter++;
	ships[id] = rbase.build_ship("fighter");
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
  choice::c_solar &c = solar_choices[id];
  research::data &r_base = players[s.owner].research_level;
  float dt = settings.dt;
  float allocated, build;

  // effect trackers
  solar_effects(id);
  s.damage_taken.clear();
  s.colonization_attempts.clear();

  if (s.owner < 0 || s.population <= 0){
    s.population = 0;
    return;
  }

  s = s.dynamics(c, dt);

  // new ships
  auto add_ship = [&](string i){
    // create ship id
    idtype sid = ship::id_counter++;

    // build ship object and add to table
    ships[sid] = r_base.build_ship(i);

    // add ship id to the solar fleet
    s.ships.insert(sid);

    // subtract from fleet growth table
    s.fleet_growth[i]--;
  };
    
  for (auto v : keywords::ship){
    while (s.fleet_growth[v] >= 1){      
      if (v == cost::keywords::key_colonizer){
	if (s.population >= r_base.colonizer_population()){
	  s.population -= r_base.colonizer_population();
	  add_ship(v);
	}else{
	  break;
	}
      }else{
	add_ship(v);
      }
    }
  }

  // new turrets
  for (auto v : keywords::turret){
    while (s.turret_growth[v] >= 1){
      s.turrets.push_back(r_base.build_turret(v));
      s.turret_growth[v]--;
    }
  }

  cout << ", result: " << s.population << endl;
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
      players[i.second.owner].research_level.develope(i.second.research);
      i.second.research = 0;
    }
  }
}

bool game_data::fleet_seen_by(idtype fid, idtype pid){
  if (!fleets.count(fid)){
    cout << "fleet seen by: not found: " << fid << endl;
    exit(-1);
  }

  fleet &f = fleets[fid];
  bool seen = f.owner == pid;

  for (auto i = fleets.begin(); i != fleets.end() && !seen; i++){
    seen |= fid != i -> first && i -> second.owner == pid && utility::l2norm(f.position - i -> second.position) < i -> second.vision;
  }

  for (auto i = solars.begin(); i != solars.end() && !seen; i++){
    seen |= i -> second.owner == pid && utility::l2norm(f.position - i -> second.position) < i -> second.compute_vision();
  }

  return seen;
}

game_data game_data::limit_to(idtype id){
  // build limited game data object;
  game_data gc;

  // load fleets
  for (auto &x : fleets){
    if (fleet_seen_by(x.first, id)) gc.fleets[x.first] = x.second;
  }

  // load solars
  for (auto &x : solars){
    bool seen = x.second.owner == id;
    for (auto i = fleets.begin(); i != fleets.end() && !seen; i++){
      seen |= i -> second.owner == id && utility::l2norm(x.second.position - i -> second.position) < i -> second.vision;
    }

    for (auto i = solars.begin(); i != solars.end() && !seen; i++){
      seen |= x.first != i -> first && i -> second.owner == id && utility::l2norm(x.second.position - i -> second.position) < i -> second.compute_vision();
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
      gc.waypoints[x.first] = x.second;
    }
  }

  // load players and settings
  gc.players = players;
  gc.settings = settings;
  gc.remove_entities = remove_entities;

  return gc;
}
