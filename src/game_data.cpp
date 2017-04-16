#include <algorithm>
#include <iostream>
#include <cmath>
#include <memory>

#include "types.h"
#include "game_data.h"
#include "utility.h"
#include "research.h"
#include "game_object.h"
#include "build_universe.h"

using namespace std;
using namespace st3;
using namespace cost;

game_data::game_data(){}

game_data::game_data(const game_data &g) {
  *this = g;
}

game_data &game_data::operator =(const game_data &g){
  allocate_grid();
  for (auto x : g.entity) entity[x.first] = x.second -> clone();

  players = g.players;
  settings = g.settings;
  remove_entities = g.remove_entities;
}

game_object::ptr game_data::get_entity(combid i){
  if (entity.count(i)){
    return entity[i];
  }else{
    cout << "game_data::get_entity: not found: " << i << endl;
    exit(-1);
  }
}

ship::ptr game_data::get_ship(combid i){
  return utility::guaranteed_cast<ship>(entity[i]);
}

fleet::ptr game_data::get_fleet(combid i){
  return utility::guaranteed_cast<fleet>(entity[i]);
}

solar::ptr game_data::get_solar(combid i){
  return utility::guaranteed_cast<solar>(entity[i]);
}

waypoint::ptr game_data::get_waypoint(combid i){
  return utility::guaranteed_cast<waypoint>(entity[i]);
}

template<typename T>
list<typename T::ptr> game_data::all(){
  list<typename T::ptr> res;

  for (auto p : entity){
    if (identifier::get_type(p.first) == T::class_id){
      res.push_back(utility::guaranteed_cast<T>(p.second));
    }
  }

  return res;
}

list<game_object::ptr> game_data::all_owned_by(idtype id){
  list<game_object::ptr> res;

  for (auto p : entity)
    if (p.second -> owner == id) res.push_back(p.second);

  return res;
}

bool game_data::target_position(combid t, point &p){
  if (entity.count(t)) {
    p = entity[t] -> position;
    return true;
  }else{
    return false;
  }
}

combid game_data::entity_at(point p){
  for (auto &x : entity){
    if (utility::l2d2(p - x.second -> position) < pow(x.second -> radius, 2)){
      return x.first;
    }
  }

  return identifier::source_none;
}

// find all entities in ball(p, r) matching condition c
list<combid> game_data::search_targets(point p, float r, target_condition c){
  list<combid> res;
    
  for (auto i : entity_grid -> search(p, r)){
    if (c.requires_target()){
      game_object::ptr x = entity[i.first];
      if (c.what == identifier::get_type(x -> id) && (c.alignment & target_condition::get_alignment(x -> owner, c.owner))){
	res.push_back(i.first);
      }
    }else{
      // return all entities if condition does not require target?
      res.push_back(i.first);
    }
  }

  return res;
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
      fleet::ptr parent = get_fleet(s -> fleet_id);
      parent -> remove_ship(i);
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

  f -> update_data(this);
  cout << "relocate ships: added fleet " << f -> id << endl;
}

// generate a fleet with given ships, set owner and fleet_id of ships
void game_data::generate_fleet(point p, idtype owner, command &c, list<combid> &sh){
  if (sh.empty()) return;

  fleet::ptr f = fleet::create();
  f -> com = c;
  f -> com.source = f -> id;
  f -> position = p;
  f -> radius = settings.fleet_default_radius;
  f -> owner = owner;

  for (auto &s : sh){
    auto sp = get_ship(s);
    f -> ships.insert(s);
    sp -> owner = owner;
    sp -> fleet_id = f -> id;
  }
  
  distribute_ships(sh, f -> position);
  f -> update_data(this);
  add_entity(f);
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
    if (!entity.count(x.first)){
      cout << "entity " << x.first << " not found for solar choice!" << endl;
      return false;
    }
    
    if (entity[x.first] -> owner != id){
      cout << "apply_choice: error: solar choice by player " << id << " for solar " << x.first << " owned by " << entity[x.first] -> owner << endl;
      return false;
    }
  }

  for (auto x : c.commands){
    if (!entity.count(x.first)){
      cout << "entity " << x.first << " not found for command!" << endl;
      return false;
    }    

    if (entity[x.first] -> owner != id){
      cout << "apply_choice: error: solar choice by player " << id << " for solar " << x.first << " owned by " << entity[x.first] -> owner << endl;
      return false;
    }
  }

  return true;
}
 
void game_data::apply_choice(choice::choice c, idtype id){
  cout << "game_data: running dummy apply choice" << endl;

  if (!validate_choice(c, id)){
    cout << "player " << id << " submitted an invalid choice!" << endl;
    exit(-1);
  }

  cout << "apply_choice for player " << id << ": inserting " << c.waypoints.size() << " waypoints:" << endl;

  // build waypoints
  for (auto &x : c.waypoints) {
    waypoint::ptr w = make_shared<waypoint>(x.second);
    add_entity(w);
  }

  // set solar choices
  for (auto &x : c.solar_choices) {
    solar::ptr s = get_solar(x.first);
    s -> choice_data = x.second;
  }

  // distribute commands
  for (auto x : c.commands){
    cout << "apply_choice: checking command key " << x.first << endl;
    commandable_object::ptr v = utility::guaranteed_cast<commandable_object>(entity[x.first]);
    v -> give_commands(x.second, this);
  }
}

void game_data::allocate_grid(){
  entity_grid = grid::tree::create();
  for (auto x : entity) entity_grid -> insert(x.first, x.second -> position);
}

void game_data::add_entity(game_object::ptr p){
  if (entity.count(p -> id)){
    cout << "add_entity: " << p -> id << ": already exists!" << endl;
    exit(-1);
  }

  cout << "add_entity: " << p -> id << " owned by " << p -> owner << endl;
  
  entity[p -> id] = p;
  p -> on_add(this);
}

void game_data::remove_entity(combid i){
  if (!entity.count(i)){
    cout << "remove_entity: " << i << ": doesn't exist!" << endl;
    exit(-1);
  }
  entity[i] -> on_remove(this);
  entity.erase(i);
  remove_entities.push_back(i);
}

void game_data::remove_units(){
  // remove entity
  for (auto i = entity.begin(); i != entity.end();){
    if (i -> second -> remove){
      remove_entity((i++) -> first);
    }else{
      i++;
    }
  }
}

// should set positions, update stats and add entities
void game_data::distribute_ships(list<combid> sh, point p){
  float density = 0.01;
  float area = sh.size() / density;
  float radius = area / (2 * M_PI);
  
  for (auto x : sh){
    ship::ptr s = get_ship(x);
    s -> position.x = utility::random_normal(p.x, radius);
    s -> position.y = utility::random_normal(p.y, radius);
    s -> current_stats = s -> base_stats;
  }
}

void game_data::increment(){
  for (auto x : entity) x.second -> pre_phase(this);
  for (auto x : entity) x.second -> move(this);
  for (auto x : entity) x.second -> interact(this);

  remove_units();
  for (auto x : entity) x.second -> post_phase(this);
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
  hm_t<idtype, combid> test_homes;
  hm_t<combid, solar> solar_data;
  hm_t<combid, ship> ship_data;
  int d_start = 10;
  vector<idtype> player_ids;
  utility::assign_keys(players, player_ids);

  for (int i = 0; i < ntest && unfairness > 0; i++){
    hm_t<combid, solar> solar_buf = build_universe::random_solars(settings);
    float u = build_universe::heuristic_homes(solar_buf, test_homes, settings, player_ids);

    if (u < unfairness){
      cout << "game_data::initialize: new best homes, u = " << u << endl;
      unfairness = u;
      solar_data = solar_buf;
      ship_data.clear();

      for (auto x : test_homes){
	solar &s = solar_data[x.second];
	s.owner = x.first;
	s.resource = initial_resources;
	s.water = 1000;
	s.space = 1000;
	s.population = 100;
	s.happiness = 1;
	ship sh = rbase.build_ship(cost::keywords::key_scout);
	sh.owner = x.first;
	s.ships.insert(sh.id);
	ship_data[sh.id] = sh;
      }
    }
  }

  // TODO: both solars get the same ship-id, though only one player
  // gets the ship
  allocate_grid();
  for (auto &s : solar_data) add_entity(solar::ptr(new solar(s.second)));
  for (auto &s : ship_data) add_entity(ship::ptr(new ship(s.second)));
}

// clean up things that will be reloaded from client
void game_data::pre_step(){
  // idle all non-idle fleets
  for (auto i : all<fleet>()){
    if (!i -> is_idle()){
      i -> com.target = identifier::target_idle;
    }
  }

  // clear waypoints, but don't list removals as client manages wp
  for (auto i : all<waypoint>()) i -> remove = true;
  remove_units();
  remove_entities.clear();
}

// pool research and remove unused waypoints
void game_data::end_step(){
  cout << "end_step:" << endl;

  bool check;
  list<combid> remove;

  for (auto i : all<waypoint>()){
    check = false;
    
    // check for fleets targeting this waypoint
    for (auto j : all<fleet>()){
      check |= j -> com.target == i -> id;
    }

    // check for waypoints with commands targeting this waypoint
    for (auto j : all<waypoint>()){
      for (auto &k : j -> pending_commands){
	check |= k.target == i -> id;
      }
    }

    if (!check) remove.push_back(i -> id);
  }

  for (auto i : remove) {
    remove_entity(i);
    remove_entities.push_back(i);
    cout << "end_step: removed " << i << endl;
  }

  // pool research
  for (auto i : all<solar>()){
    if (i -> owner > -1){
      players[i -> owner].research_level.develope(i -> research);
      i -> research = 0;
    }
  }
}

bool game_data::entity_seen_by(combid id, idtype pid){
  if (!entity.count(id)){
    cout << "entity seen by: not found: " << id << endl;
    exit(-1);
  }

  auto landed_ship = [this] (game_object::ptr s) -> bool{
    return identifier::get_type(s -> id) == ship::class_id && get_ship(s -> id) -> fleet_id == identifier::source_none;
  };
  
  game_object::ptr x = entity[id];

  // always see owned entities
  if (x -> owner == pid) return true;

  // never see opponent waypoints
  if (identifier::get_type(id) == waypoint::class_id) return false;

  // don't see ships if they're not in a fleet
  if (landed_ship(x)) return false;

  auto buf = all_owned_by(pid);
  bool seen = false;

  for (auto i = buf.begin(); i != buf.end() && !seen; i++){
    // don't use ships to spot if they're not in a fleet
    game_object::ptr s = *i;
    if (landed_ship(s)) continue;
    seen |= s -> owner == pid && utility::l2d2(x -> position - s -> position) < pow(s -> vision(), 2);
  }

  return seen;
}

game_data game_data::limit_to(idtype id){
  // build limited game data object;
  game_data gc;

  cout << "game data: limit to: " << id << endl;

  gc.allocate_grid();
  for (auto i : entity){
    if (entity_seen_by(i.first, id)){
      gc.add_entity(i.second);
      cout << " :: adding entity: " << i.first << endl;
    }
  }

  // load players and settings
  gc.players = players;
  gc.settings = settings;
  gc.remove_entities = remove_entities;

  return gc;
}
