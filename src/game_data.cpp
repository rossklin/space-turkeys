#include <algorithm>
#include <iostream>
#include <cmath>
#include <memory>
#include <cassert>

#include "types.h"
#include "game_data.h"
#include "utility.h"
#include "research.h"
#include "game_object.h"
#include "build_universe.h"
#include "com_server.h"
#include "upgrades.h"

using namespace std;
using namespace st3;
using namespace cost;

game_data::game_data(){
  entity_grid = 0;
}

game_data::~game_data(){
  clear_entities();
}

void game_data::allocate_grid(){
  clear_entities();
  entity_grid = grid::tree::create();
  for (auto x : entity) entity_grid -> insert(x.first, x.second -> position);
}

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
// TODO: match against player vision matrix
list<combid> game_data::search_targets_nophys(combid self_id, point p, float r, target_condition c){
  list<combid> res;
  game_object::ptr self = get_entity(self_id);
    
  for (auto i : entity_grid -> search(p, r)){
    auto e = get_entity(i.first);
    if (evm[self-> owner].count(i.first) && e -> id != self_id && c.valid_on(e)) res.push_back(e -> id);
  }

  return res;
}

// find all entities in ball(p, r) matching condition c
list<combid> game_data::search_targets(combid self_id, point p, float r, target_condition c){
  list<combid> res;
  game_object::ptr self = get_entity(self_id);
  if (!self -> isa(physical_object::class_id)) {
    throw runtime_error("Non-physical entity called search targets: " + self_id);
  }
  physical_object::ptr s = utility::guaranteed_cast<physical_object>(self);
    
  for (auto i : entity_grid -> search(p, r)){
    auto e = get_entity(i.first);
    if (s -> can_see(e) && e -> id != self_id && c.valid_on(e)) res.push_back(e -> id);
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
    c.origin = f -> com.origin;
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

  f -> update_data(this, true);
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
  add_entity(f);
  f -> update_data(this, true);
}

void game_data::apply_choice(choice::choice c, idtype id){
  cout << "apply_choice: player " << id << endl;

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

    f -> update_data(this, true);

    cout << "apply_choice: player " << id << ": added " << x.first << endl;
  }

  // research choice: evaluated before solar choice so dependencies
  // match those on client side
  
  // validate
  if (c.research.length() > 0) {
    list<string> available_techs = players[id].research_level.available();
    bool ok = false;
    for (auto t : available_techs) ok |= t == c.research;
    if (!ok) {
      throw runtime_error("Invalid research choice submitted by player " + to_string(id) + ": " + c.research);
    }
  }

  // apply
  if (c.research.length() > 0){
    research::tech t = research::data::table().at(c.research);
    players[id].research_level.accumulated -= t.cost_time;
    players[id].research_level.researched.insert(t.name);
  }

  // solar choices: require research to be applied
  for (auto &x : c.solar_choices){
    // validate
    auto e = get_entity(x.first);
    
    if (e -> owner != id){
      throw runtime_error("validate_choice: error: solar choice by player " + to_string(id) + " for " + x.first + " owned by " + to_string(e -> owner));
    }
    
    if (!e -> isa(solar::class_id)){
      throw runtime_error("validate_choice: error: solar choice by player " + to_string(id) + " for " + x.first + ": not a solar!");
    }

    if (!x.second.development.empty()){
      list<string> av = get_solar(x.first) -> available_facilities(players[id].research_level);
      if (find(av.begin(), av.end(), x.second.development) == av.end()) {
	throw runtime_error("validate_choice: error: solar choice contained invalid development: " + x.second.development);
      }
    }

    // apply
    solar::ptr s = get_solar(x.first);
    s -> choice_data = x.second;

    if (!x.second.development.empty()) {
      s -> develop(x.second.development);
    }
  }

  // commands: validate
  for (auto x : c.commands){
    auto e = get_entity(x.first);
    if (e -> owner != id){
      throw runtime_error("validate_choice: error: command by player " + to_string(id) + " for " + x.first + " owned by " + to_string(e -> owner));
    }
  }

  // apply
  for (auto x : c.commands){
    commandable_object::ptr v = utility::guaranteed_cast<commandable_object>(get_entity(x.first));
    v -> give_commands(x.second, this);
  }
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
    s -> set_stats(s -> base_stats);
    entity_grid -> insert(s -> id, s -> position);
  }
}

void game_data::collide_ships(id_pair x) {
  ship::ptr s = get_ship(x.a);
  ship::ptr t = get_ship(x.b);

  point delta = s -> position - t -> position;
  float smaller_mass = fmin(t -> stats[sskey::key::mass], s -> stats[sskey::key::mass]);
  point velocity_self(t -> stats[sskey::key::speed] * cos(t -> angle), t -> stats[sskey::key::speed] * sin(t -> angle));
  point velocity_other(s -> stats[sskey::key::speed] * cos(s -> angle), s -> stats[sskey::key::speed] * sin(s -> angle));
  float collision_energy = 0.5 * smaller_mass * utility::l2d2(velocity_other - velocity_self);
  float mass_ratio = t -> stats[sskey::key::mass] / s -> stats[sskey::key::mass];
  point new_velocity = utility::scale_point(velocity_self, mass_ratio) + utility::scale_point(velocity_other, 1/mass_ratio);
  float new_angle = utility::point_angle(new_velocity);
  float new_speed = utility::l2norm(new_velocity);

  // merge speed and angle
  t -> stats[sskey::key::speed] = new_speed;
  s -> stats[sskey::key::speed] = new_speed;
  t -> angle = new_angle;
  s -> angle = new_angle;

  // push ships apart a bit
  t -> position += utility::scale_point(utility::normv(utility::point_angle(-delta)), 3);
  s -> position += utility::scale_point(utility::normv(utility::point_angle(delta)), 3);

  // damage ships
  s -> receive_damage(t, utility::random_uniform(0, collision_energy));
  t -> receive_damage(s, utility::random_uniform(0, collision_energy));
}

void game_data::rebuild_evm() {
  evm.clear();

  // rebuild vision matrix
  for (auto x : entity) {
    if (x.second -> is_active()){
      // entity sees itself
      evm[x.second -> owner].insert(x.first);

      // only physical entities can see others
      if (x.second -> isa(physical_object::class_id)) {
	physical_object::ptr e = utility::guaranteed_cast<physical_object>(x.second);
	for (auto i : entity_grid -> search(e -> position, e -> vision())){
	  game_object::ptr t = get_entity(i.first);
	  // only see other players' entities if they are physical and active
	  if (t -> isa(physical_object::class_id) && t -> is_active() && e -> can_see(t)) evm[e -> owner].insert(t -> id);
	}
      }
    }
  }
}

solar::ptr game_data::closest_solar(point p, idtype id) {
  solar::ptr s = 0;

  try {
    s = utility::value_min<solar::ptr>(all<solar>(), [this, p, id] (solar::ptr t) -> float {
	if (t -> owner != id) {
	  return INFINITY;
	}else{
	  return utility::l2d2(t -> position - p);
	}
      });
  } catch (exception &e) {
    // player doesn't own any solars
    return 0;
  }

  return s;
}

void game_data::increment(){
  remove_entities.clear();
  interaction_buffer.clear();
  rebuild_evm();

  // update entities and compile interactions
  for (auto x : entity) if (x.second -> is_active()) x.second -> pre_phase(this);
  for (auto x : entity) if (x.second -> is_active()) x.second -> move(this);

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

  // perform collisions
  for (auto x : collision_buffer) collide_ships(x);

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

  cost::res_t initial_resources;
  for (auto v : keywords::resource) initial_resources[v] = 1000;
  
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
	s.available_resource = initial_resources;
	s.water = 1000;
	s.space = 1000;
	s.population = 100;
	s.happiness = 1;
	s.ecology = 1;
	s.dt = settings.dt;
	s.radius = settings.solar_maxrad;
	ship sh = rbase.build_ship(ship::starting_ship, &s);
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
  hm_t<idtype, hm_t<string, int> > level;
  for (auto i : all<solar>()){
    if (i -> owner > -1){
      pool[i -> owner] += i -> research_points;
      for (auto &f : i -> development) {
	level[i -> owner][f.first] = max(level[i -> owner][f.first], f.second.level);
      }
      i -> research_points = 0;
    }
  }

  for (auto x : players) {
    idtype id = x.first;
    players[id].research_level.accumulated += pool[id];
    players[id].research_level.facility_level = level[id];
  }
}

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

// limit_to without deallocating
void entity_package::limit_to(idtype id){
  list<combid> remove_buf;
  for (auto i : entity) {
    if (!(i.second -> owner == id || evm[id].count(i.first))) remove_buf.push_back(i.first);
  }
  for (auto i : remove_buf) entity.erase(i);
}

void entity_package::copy_from(const game_data &g){
  if (entity.size()) throw runtime_error("Attempting to assign game_data: has entities!");

  for (auto x : g.entity) entity[x.first] = x.second -> clone();
  players = g.players;
  settings = g.settings;
  remove_entities = g.remove_entities;
  evm = g.evm;
}

void entity_package::clear_entities(){
  for (auto x : entity) delete x.second;
  entity.clear();
}

// load data tables and confirm data references
void game_data::confirm_data() {
  auto &itab = interaction::table();
  auto &utab = upgrade::table();
  auto &stab = ship::table();
  auto &rtab = research::data::table();
  auto &dtab = solar::facility_table();

  auto check_ship_upgrades = [&utab, &stab] (hm_t<string, set<string> > u) {
    for (auto &x : u) {
      for (auto v : x.second) assert(utab.count(v));
      if (x.first == research::upgrade_all_ships) continue;
      if (x.first[0] == '!' || x.first[0] == '#' || x.first[0] == '[') continue;
      assert(stab.count(x.first));
    }
  };

  // validate upgrades
  for (auto &u : utab) for (auto i : u.second.inter) assert(itab.count(i));

  // validate ships
  for (auto &s : stab) {
    if (!s.second.depends_tech.empty()) assert(rtab.count(s.second.depends_tech));
    for (auto &u : s.second.upgrades) assert(utab.count(u));
  }
  assert(stab.count(ship::starting_ship));

  // validate technologies
  for (auto &t : rtab) {
    for (auto v : t.second.depends_techs) assert(rtab.count(v));
    check_ship_upgrades(t.second.ship_upgrades);
  }

  // validate facilities
  for (auto &f : dtab) {
    check_ship_upgrades(f.second.ship_upgrades);
    for (auto &d : f.second.depends_facilities) assert(dtab.count(d.first));
    for (auto &d : f.second.depends_techs) assert(rtab.count(d));
  }
}

template list<ship::ptr> game_data::all<ship>();
