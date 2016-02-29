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

template<typename T>
T attempt_cast(game_object::ptr p){
  if (p == 0){
    cout << "attempt_cast: null pointer!" << endl;
    exit(-1);
  }
  
  T res = dynamic_cast<T>(p);

  if (res){
    return res;
  }else{
    cout << "Failed to downcast entity " << p -> id << endl;
    exit(-1);
  }
}

ship::ptr game_data::get_ship(combid i){
  return attempt_cast<ship::ptr>(entity[i]);
}

fleet::ptr game_data::get_fleet(combid i){
  return attempt_cast<fleet::ptr>(entity[i]);
}

solar::ptr game_data::get_solar(combid i){
  return attempt_cast<solar::ptr>(entity[i]);
}

waypoint::ptr game_data::get_waypoint(combid i){
  return attempt_cast<waypoint::ptr>(entity[i]);
}

bool game_data::target_position(combid t, point &p){
  if (entity.count(t)) {
    p = entity[t].position;
    return true;
  }else{
    return false;
  }
}

combid game_data::entity_at(point p){
  for (auto &x : entity){
    if (utility::l2d2(p - x.second.position) < pow(x.second.radius, 2)){
      return x.first;
    }
  }

  return identifier::source_none;
}

// create a new fleet and add ships from command c
void game_data::relocate_ships(command &c, set<combid> &sh, idtype owner){
  fleet::ptr f;
  combid source_id;
  set<combid> fleet_buf;

  // check if ships fill exactly one fleet
  for (auto i : sh) {
    ship::ptr s = get_ship(i);
    fleet_buf.insert(source_id = s -> fleet_id);
  }
  
  bool reassign = false;
  if (fleet_buf.size() == 1){
    f = get_fleet(source_id);
    reassign = f -> ships == sh;
  }

  if (reassign){
    cout << "relocate: reassign" << endl;
    c.source = f -> com.source;
    f -> com = c;
  }else{
    f = fleet::create();

    f -> com = c;
    f -> owner = owner;
    f -> update_counter = 0;
    f -> com.source = f -> id;

    // clear ships from parent fleets
    for (auto i : sh) {
      ship::ptr s = get_ship(i);
      fleet:::ptr parent = get_fleet(s -> fleet_id);
      parent -> ships.erase(i);
    }

    // set new fleet id
    for (auto i : sh) {
      ship::ptr s = get_ship(i);
      s -> fleet_id = f -> id;
    }

    // add the ships
    f -> ships = sh;

    // add the fleet 
    add_entity(f);
  }

  f -> update_data();
  cout << "relocate ships: added fleet " << fid << endl;
}

// generate a fleet with given ships, set owner and fleet_id of ships
void game_data::generate_fleet(point p, idtype owner, command &c, set<ship::ptr> &sh){
  if (sh.empty()) return;

  fleet::ptr f = fleet::create();
  f -> com = c;
  f -> com.source = f -> id;
  f -> position = p;
  f -> radius = settings.fleet_default_radius;
  f -> owner = owner;

  for (auto s : sh){
    f -> ships.insert(s -> id);
    s -> owner = owner;
    s -> fleet_id = f -> id;
  }
  
  distribute_ships(sh, f -> position);
  f -> update_data();
}

void game_data::set_fleet_commands(idtype id, list<command> commands){
  fleet::ptr s = get_fleet(id);

  // split fleets
  // evaluate commands in random order
  vector<command> buf(commands.begin(), commands.end());
  random_shuffle(buf.begin(), buf.end());

  for (auto &x : buf){
    if (x.ships == s -> ships){
      // maintain id for trackability
      // if all ships were assigned, break.
      s -> com.target = x.target;
      s -> com.action = x.action;
      break;
    }else{
      fleet::ptr f = fleet::create();
      f -> com = x;
      f -> com.source = f -> id;
      f -> position = s -> position;
      f -> radius = s -> radius;
      f -> owner = s -> owner;
      for (auto i : x.ships){
	if (s -> ships.count(i)){
	  ship::ptr buf = get_ship(i);
	  buf -> fleet_id = f -> id;
	  s -> ships.erase(i);
	  f -> ships.insert(i);
	}
      }

      add_entity(f);
    }
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

bool game_data::validate_choice(choice::choice c, idtype id){
  cout << "game_data: validate choice" << endl;

  for (auto &x : c.waypoints){
    if (identifier::get_waypoint_owner(x.first) != id){
      cout << "apply_choice: player " << id << " tried to insert waypoint owned by " << identifier::get_waypoint_owner(x.first) << endl;
      return false;
    }
  }

  for (auto &x : c.solar_choices){
    if (entity[x.first] -> owner != id){
      cout << "apply_choice: error: solar choice by player " << id << " for solar " << x.first << " owned by " << entity[x.first] -> owner << endl;
      exit(-1);
    }
  }

  for (auto x : c.commands){
    cout << "apply_choice: checking command key " << x.first << endl;

    if (entity[x.first] -> owner != id){
      cout << "apply_choice: error: solar choice by player " << id << " for solar " << x.first << " owned by " << entity[x.first] -> owner << endl;
      return false;
    }
  }

  return true;
}
 
void game_data::apply_choice(choice::choice c, idtype id){
  cout << "game_data: running dummy apply choice" << endl;

  if (!validate_choice(c)){
    cout << "player " << id << " submitted an invalid choice!" << endl;
    exit(-1);
  }

  cout << "apply_choice for player " << id << ": inserting " << c.waypoints.size() << " waypoints:" << endl;

  for (auto &x : c.waypoints) waypoints[x.first] = make_shared(x.second);
  for (auto &x : c.solar_choices) solar_choices[x.first] = x.second;

  for (auto x : c.commands){
    cout << "apply_choice: checking command key " << x.first << endl;

    if (identifier::get_type(x.first) == identifier::solar){
      set_solar_commands(x.first, x.second);
    }else if (identifier::get_type(x.first) == identifier::fleet){
      set_fleet_commands(x.first, x.second);
    }else{
      cout << "apply_choice: invalid source: '" << x.first << "'" << endl;
      exit(-1);
    }
  }
}

void game_data::allocate_grid(){
  entity_grid = grid::tree::create();
  for (auto x : entity) entity_grid -> insert(x.first, x.second -> position);
}

void game_data::remove_units(){
  // remove entity
  for (auto i = entity.begin(); i != entity.end();){
    if (i -> second -> remove){
      i -> second -> on_remove();
      entity.erase((i++) -> first);
    }else{
      i++;
    }
  }
}

void game_data::increment(){
  for (auto x : entity) x.second -> pre_phase(this);
  for (auto x : entity) x.second -> move(this);
  for (auto x : entity) x.second -> interact(this);

  remove_units();
  for (auto x : entity) x.second -> post_phase(this);
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
      ships[i.second].remove = true;
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
      t.remove = true;
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
