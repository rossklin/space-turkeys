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
#include "com_server.h"

using namespace std;
using namespace st3;
using namespace cost;

game_data::game_data(){
  entity_grid = 0;
}

game_data::~game_data(){
  clear_entities();
}

void game_data::clear_entities(){
  for (auto x : entity) delete x.second;
  entity.clear();
}

void game_data::allocate_grid(){
  clear_entities();
  entity_grid = grid::tree::create();
  for (auto x : entity) entity_grid -> insert(x.first, x.second -> position);
}

// void game_data::assign(const game_data &g){
//   if (entity_grid || entity.size()) throw runtime_error("Attempting to assign to game_data: already allcoated!");

//   entity_grid = g.entity_grid -> clone();
//   for (auto x : g.entity) entity[x.first] = x.second -> clone();
//   players = g.players;
//   settings = g.settings;
//   remove_entities = g.remove_entities;
// }

ship::ptr game_data::get_ship(combid i){
  return utility::guaranteed_cast<ship>(get_entity(i));
}

fleet::ptr game_data::get_fleet(combid i){
  return utility::guaranteed_cast<fleet>(get_entity(i));
}

solar::ptr game_data::get_solar(combid i){
  return utility::guaranteed_cast<solar>(get_entity(i));
}

waypoint::ptr game_data::get_waypoint(combid i){
  return utility::guaranteed_cast<waypoint>(get_entity(i));
}

template<typename T>
list<typename T::ptr> game_data::all(){
  list<typename T::ptr> res;

  for (auto p : entity){
    if (p.second -> isa(T::class_id)){
      res.push_back(utility::guaranteed_cast<T>(p.second));
    }
  }

  return res;
}

bool game_data::target_position(combid t, point &p){
  if (entity.count(t)) {
    p = get_entity(t) -> position;
    return true;
  }else{
    return false;
  }
}

// find all entities in ball(p, r) matching condition c
list<combid> game_data::search_targets(combid self_id, point p, float r, target_condition c){
  list<combid> res;
    
  for (auto i : entity_grid -> search(p, r)){
    auto e = get_entity(i.first);
    if (e -> is_active() && e -> id != self_id && c.valid_on(e)) res.push_back(e -> id);
  }

  return res;
}

// create a new fleet and add ships from command c
void game_data::relocate_ships(command c, set<combid> &sh, idtype owner){
  fleet::ptr f;
  combid source_id;
  set<combid> fleet_buf;

  // check if ships fill exactly one fleet
  for (auto i : sh) {
    ship::ptr s = get_ship(i);
    if (s -> has_fleet()) fleet_buf.insert(source_id = s -> fleet_id);
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
    f = fleet::create(fleet::server_pid);

    f -> com = c;
    f -> owner = owner;
    f -> update_counter = 0;
    f -> com.source = f -> id;

    // clear ships from parent fleets
    for (auto i : sh) {
      ship::ptr s = get_ship(i);
      if (s -> has_fleet()){
	fleet::ptr parent = get_fleet(s -> fleet_id);
	parent -> remove_ship(i);
      }
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

  fleet::ptr f = fleet::create(fleet::server_pid);
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
  cout << "game_data: validate choice: solar_choices: " << c.solar_choices.size() << endl;

  // research choice
  if (c.research.identifier.length() > 0) {
    list<string> available_techs = players[id].research_level.available();
    bool ok = false;
    for (auto t : available_techs) ok |= t == c.research.identifier;
    if (!ok) {
      cout << "Invalid research choice submitted by player " << id << ": " << c.research.identifier << endl;
      return false;
    }
  }

  // solar choices
  for (auto &x : c.solar_choices){
    auto e = get_entity(x.first);
    
    if (e -> owner != id){
      cout << "apply_choice: error: solar choice by player " << id << " for solar " << x.first << " owned by " << e -> owner << endl;
      return false;
    }
    
    if (!e -> isa(solar::class_id)){
      cout << "apply_choice: error: solar choice by player " << id << " for " << x.first << ": not a solar!" << endl;
      return false;
    }
  }

  // commands
  for (auto x : c.commands){
    auto e = get_entity(x.first);

    if (e -> owner != id){
      cout << "apply_choice: error: command by player " << id << " for " << x.first << " owned by " << e -> owner << endl;
      return false;
    }
  }

  return true;
}
 
void game_data::apply_choice(choice::choice c, idtype id){
  cout << "apply_choice: player " << id << ": waypoints before: " << endl;
  for (auto w : all<waypoint>()) cout << w -> id << endl;

  // build waypoints and fleets before validating the choice, so that
  // commands based there can be validated
  for (auto &x : c.waypoints){
    if (identifier::get_multid_owner(x.second.id) != id){
      throw runtime_error("apply_choice: player " + to_string(id) + " tried to insert waypoint owned by " + to_string(identifier::get_multid_owner(x.second.id)));
    }
    add_entity(x.second.clone());
    cout << "apply_choice: player " << id << ": added " << x.first << endl;
  }

  for (auto &x : c.fleets){
    string sym = identifier::get_multid_owner_symbol(x.second.id);
    if (sym != to_string(id) && sym != "S"){
      throw runtime_error("apply_choice: player " + to_string(id) + " tried to insert fleet owned by " + sym);
    }
    
    x.second.owner = id;
    add_entity(x.second.clone());

    auto f = get_fleet(x.first);
    for (auto sid : f -> ships) {
      ship::ptr s = get_ship(sid);
      if (s -> owner == id) {
	s -> fleet_id = f -> id;
      }else{
	throw runtime_error("apply_choice: player " + to_string(id) + " tried to construct a fleet with a non-owned ship!");
      }
    }

    f -> update_counter = 0;
    f -> update_data(this);

    cout << "apply_choice: player " << id << ": added " << x.first << endl;
  }

  if (!validate_choice(c, id)) throw runtime_error("player " + to_string(id) + " submitted an invalid choice!");

  // research choice
  if (c.research.identifier.length() > 0){
    research::tech t = players[id].research_level.tree[c.research.identifier];
    players[id].research_level.accumulated -= t.cost;
    players[id].research_level.researched.insert(t.name);
  }

  // set solar choices
  for (auto &x : c.solar_choices) {
    solar::ptr s = get_solar(x.first);
    s -> choice_data = x.second;
  }

  // distribute commands
  for (auto x : c.commands){
    commandable_object::ptr v = utility::guaranteed_cast<commandable_object>(get_entity(x.first));
    v -> give_commands(x.second, this);
  }

  cout << "apply_choice: player " << id << ": waypoints after: " << endl;
  for (auto w : all<waypoint>()) cout << w -> id << endl;
}

void game_data::add_entity(game_object::ptr p){
  if (entity.count(p -> id)) throw runtime_error("add_entity: already exists: " + p -> id);  
  entity[p -> id] = p;
  p -> on_add(this);
}

void game_data::remove_entity(combid i){
  if (!entity.count(i)) throw runtime_error("remove_entity: " + i + ": doesn't exist!");
  get_entity(i) -> on_remove(this);
  delete get_entity(i);
  entity.erase(i);
  remove_entities.push_back(i);
}

void game_data::remove_units(){
  auto buf = entity;
  for (auto i : buf) {
    if (i.second -> remove) {
      remove_entity(i.first);
    }
  }
}

// should set positions, update stats and add entities
void game_data::distribute_ships(list<combid> sh, point p){
  float density = 0.01;
  float area = sh.size() / density;
  float radius = sqrt(area / M_PI);
  
  for (auto x : sh){
    ship::ptr s = get_ship(x);
    s -> position.x = utility::random_normal(p.x, radius);
    s -> position.y = utility::random_normal(p.y, radius);
    s -> current_stats = s -> base_stats;
    entity_grid -> insert(s -> id, s -> position);
  }
}

void game_data::increment(){
  remove_entities.clear();
  interaction_buffer.clear();

  // update entities and compile interactions
  for (auto x : entity) if (x.second -> is_active()) x.second -> pre_phase(this);
  for (auto x : entity) if (x.second -> is_active()) x.second -> move(this);
  for (auto x : all<physical_object>()) if (x -> is_active()) x -> interact(this);

  // perform interactions
  random_shuffle(interaction_buffer.begin(), interaction_buffer.end());
  auto itab = interaction::table();
  for (auto x : interaction_buffer) {
    interaction i = itab[x.interaction];
    game_object::ptr s = get_entity(x.source);
    game_object::ptr t = get_entity(x.target);
    if (i.condition.owned_by(s -> owner).valid_on(t)) {
      i.perform(s, t, this);
    }
  }

  // remove units twice, since removing ships can cause fleets to set
  // the remove flag
  remove_units();
  remove_units();
  
  for (auto x : entity) if (x.second -> is_active()) x.second -> post_phase(this);
}

void game_data::build_players(hm_t<int, server::client_t*> clients){
  // build player data
  vector<sint> colbuf = utility::different_colors(clients.size());
  int i = 0;
  player p;
  for (auto x : clients){
    p.name = x.second -> name;
    p.color = colbuf[i++];
    players[x.first] = p;
  }
}

// players and settings should be set before build is called
void game_data::build(){
  static research::data rbase;
  
  if (players.empty()) throw runtime_error("game_data: build: no players!");

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
	s.ecology = 1;
	s.dt = settings.dt;
	ship sh = rbase.build_ship(cost::keywords::key_scout);
	sh.is_landed = true;
	sh.owner = x.first;
	s.ships.insert(sh.id);
	ship_data[sh.id] = sh;
      }
    }
  }

  allocate_grid();
  for (auto &s : solar_data) add_entity(solar::ptr(new solar(s.second)));
  for (auto &s : ship_data) add_entity(ship::ptr(new ship(s.second)));
}

// clean up things that will be reloaded from client
void game_data::pre_step(){
  // // idle all non-idle fleets
  // for (auto i : all<fleet>()) i -> com.action = fleet_action::idle;

  // clear waypoints and fleets, but don't list removals as client
  // manages them
  for (auto i : all<waypoint>()) i -> remove = true;
  for (auto i : all<fleet>()) i -> remove = true;
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
    cout << "end_step: removed " << i << endl;
  }

  // pool research
  hm_t<idtype, float> pool;
  hm_t<idtype, int> level;
  for (auto i : all<solar>()){
    if (i -> owner > -1){
      pool[i -> owner] += i -> research;
      level[i -> owner] = max(level[i -> owner], (int)i -> sector[cost::keywords::key_research]);
      i -> research = 0;
    }
  }

  for (auto x : players) {
    idtype id = x.first;
    players[id].research_level.accumulated += pool[id];
    players[id].research_level.facility_level = level[id];
  }
}

// void game_data::limit_to(idtype id){
//   auto re = remove_entities;
//   list<combid> remove_buf;
//   for (auto i : entity) {
//     if (!entity_seen_by(i.first, id)) remove_buf.push_back(i.first);
//   }

//   for (auto i : remove_buf) remove_entity(i);
//   remove_entities = re;
// }

game_object::ptr entity_package::get_entity(combid i){
  if (entity.count(i)){
    return entity[i];
  }else{
    throw runtime_error("game_data::get_entity: not found: " + i);
  }
}

list<game_object::ptr> entity_package::all_owned_by(idtype id){
  list<game_object::ptr> res;

  for (auto p : entity) {
    if (p.second -> owner == id) res.push_back(p.second);
  }

  return res;
}

bool entity_package::entity_seen_by(combid id, idtype pid){
  if (!entity.count(id)) return false;
  game_object::ptr x = get_entity(id);

  // always see owned entities
  if (x -> owner == pid) return true;

  // never see opponent waypoints or fleets
  if (x -> isa(waypoint::class_id) || x -> isa(fleet::class_id)) return false;

  // don't see entities that aren't active
  if (!x -> is_active()) return false;

  for (auto s : all_owned_by(pid)) {
    if (!s -> is_active()) continue;
    if (utility::l2d2(x -> position - s -> position) < pow(s -> vision(), 2)) return true;
  }

  return false;
}

// limit_to without deallocating
void entity_package::limit_to(idtype id){
  list<combid> remove_buf;
  for (auto i : entity) {
    if (!entity_seen_by(i.first, id)) remove_buf.push_back(i.first);
  }
  for (auto i : remove_buf) entity.erase(i);
}

template list<ship::ptr> game_data::all<ship>();
