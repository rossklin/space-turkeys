#include <algorithm>
#include <iostream>
#include <cmath>
#include <memory>
#include <cassert>
#include <queue>

#include "types.h"
#include "game_data.h"
#include "utility.h"
#include "research.h"
#include "game_object.h"
#include "com_server.h"
#include "upgrades.h"
#include "animation_data.h"

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
}

void game_data::rehash_grid() {
  entity_grid -> clear();
  list<combid> keys = utility::get_map_keys(entity);
  vector<combid> vkeys(keys.begin(), keys.end());
  random_shuffle(vkeys.begin(), vkeys.end());
  for (auto eid : vkeys) {
    game_object::ptr x = get_entity(eid);
    if (x -> is_active()) entity_grid -> insert(eid, x -> position);
  }
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

bool game_data::target_position(combid t, point &p){
  if (entity.count(t)) {
    p = get_entity(t) -> position;
    return true;
  }else{
    return false;
  }
}

// find all entities in ball(p, r) matching condition c
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
    throw logical_error("Non-physical entity called search targets: " + self_id);
  }
  physical_object::ptr s = utility::guaranteed_cast<physical_object>(self);
    
  for (auto i : entity_grid -> search(p, r)){
    auto e = get_entity(i.first);
    if (s -> can_see(e) && e -> id != self_id && c.valid_on(e)) res.push_back(e -> id);
  }

  return res;
}

list<idtype> game_data::terrain_at(point p, float r) {
  list<idtype> res;
  for (auto &x : terrain) {
    int j = x.second.triangle(p, r);
    if (j > -1) res.push_back(j);
  }
  return res;
}

list<point> game_data::get_path(point a, point b, int d) {
  if (d > 10) {
    server::log("get_path: max recursion depth reached!", "warning");
    return {};
  }

  if (terrain_at(a, 1).size() || terrain_at(b, 1).size()) {
    server::log("get_path: end point covered by terrain!", "warning");
    return {};
  }
  
  auto first_intersect = [this] (point a, point b) {

    // find first intersected terrain object
    list<int> tids;
    for (auto &x : terrain) {
      for (int i = 0; i < x.second.border.size(); i++) {
	point p1 = x.second.get_vertice(i);
	point p2 = x.second.get_vertice(i+1);
	if (utility::line_intersect(a, b, p1, p2)) {
	  tids.push_back(x.first);
	  break;
	}
      }
    }

    // find starting point
    int tid = -1;
    float closest = INFINITY;

    for (auto j : tids) {
      for (int i = 0; i < terrain[j].border.size(); i++) {
	float d = utility::l2norm(terrain[j].get_vertice(i) - a);
	if (d < closest) {
	  closest = d;
	  tid = j;
	}
      }
    }

    return tid;
  };

  // find a path around the polygon
  auto path_around = [] (terrain_object obj, point a, point b, int dir) {
    list<point> path;
    path.push_back(a);

    int pid = -1;
    int n = obj.border.size();
    float best = INFINITY;
    for (int i = 0; i < n; i++) {
      float d = utility::l2norm(obj.get_vertice(i) - a);
      if (d < best) {
	best = d;
	pid = i;
      }
    }

    auto visible_from = [obj, b, n] (int pid) {
      for (int i = 1; i < n - 1; i++) {
	point p1 = obj.get_vertice(pid + i);
	point p2 = obj.get_vertice(pid + i + 1);
	if (utility::line_intersect(obj.get_vertice(pid), b, p1, p2)) return false;
      }
      return true;
    };

    point p = obj.get_vertice(pid);
    path.push_back(p + utility::normalize_and_scale(p - obj.center, 10));
    do {
      pid = utility::int_modulus(pid + dir, n);
      p = obj.get_vertice(pid);
      path.push_back(p + utility::normalize_and_scale(p - obj.center, 10));
    } while (!visible_from(pid));

    return path;
  };

  int tid = first_intersect(a, b);

  if (tid == -1) {
    return list<point>(1, b);
  }

  // join path around with remaining path
  list<point> path_left = path_around(terrain[tid], a, b, -1);
  list<point> path_right = path_around(terrain[tid], a, b, 1);
  list<point> sub_left = get_path(path_left.back(), b, d + 1);
  list<point> sub_right = get_path(path_right.back(), b, d + 1);
  if (sub_left.size()) {
    path_left.splice(path_left.end(), sub_left);
  } else {
    path_left.clear();
  }

  if (sub_right.size()) {
    path_right.splice(path_right.end(), sub_right);
  } else {
    path_right.clear();
  }

  // optimize the path
  auto fix_path = [this, first_intersect] (list<point> path) {
    bool did_fix = true;
    int n = path.size();

    if (n < 3) return path;

    while (did_fix) {
      did_fix = false;
      auto start = path.begin();
      auto check = start;
      check++;
      if (check == path.end()) break;
      check++;
      
      while (first_intersect(*start, *check) > -1 && check != path.end()) {
	start++;
	check++;
      }
      
      while (first_intersect(*start, *check) == -1 && check != path.end()) {
	did_fix = true;
	check++;
      }

      check--;
      start++;
      path.erase(start, check);
    }

    // finally, remove starting point
    path.pop_front();
    return path;
  };

  auto count_path = [] (list<point> path) {
    if (path.size() < 3) return path.size() ? utility::l2norm(path.back() - path.front()) : INFINITY;
    
    auto i = path.begin();
    auto j = i;
    j++;

    float d = 0;
    while (j != path.end()) {
      d += utility::l2norm((*j) - (*i));
      i++;
      j++;
    }

    return d;
  };

  path_right = fix_path(path_right);
  path_left = fix_path(path_left);

  if (count_path(path_right) < count_path(path_left)) {
    return path_right;
  } else {
    return path_left;
  }
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
  float ta = utility::random_uniform(0, 2 * M_PI);
  point tp;
  if (target_position(c.target, tp)) ta = utility::point_angle(tp - p);

  for (auto &s : sh){
    auto sp = get_ship(s);
    f -> ships.insert(s);
    sp -> owner = owner;
    sp -> fleet_id = f -> id;
    sp -> angle = ta;
  }
  
  distribute_ships(sh, f -> position);
  add_entity(f);
  f -> heading = f -> position;
  f -> update_data(this, true);
}

void game_data::apply_choice(choice::choice c, idtype id){
  // build waypoints and fleets before validating the choice, so that
  // commands based there can be validated
  for (auto &x : c.waypoints){
    if (identifier::get_multid_owner(x.second.id) != id){
      throw player_error("apply_choice: player " + to_string(id) + " tried to insert waypoint owned by " + to_string(identifier::get_multid_owner(x.second.id)));
    }
    add_entity(x.second.clone());
  }

  for (auto &x : c.fleets){
    string sym = identifier::get_multid_owner_symbol(x.second.id);
    if (sym != to_string(id) && sym != "S"){
      throw player_error("apply_choice: player " + to_string(id) + " tried to insert fleet owned by " + sym);
    }
    
    x.second.owner = id;
    add_entity(x.second.clone());

    auto f = get_fleet(x.first);
    for (auto sid : f -> ships) {
      ship::ptr s = get_ship(sid);
      if (s -> owner == id) {
	s -> fleet_id = f -> id;
      }else{
	throw player_error("apply_choice: player " + to_string(id) + " tried to construct a fleet with a non-owned ship!");
      }
    }

    // idle the fleet so it only acts if the client updates the command
    f -> set_idle();
    f -> update_data(this, true);
  }

  // research
  if (c.research.length() > 0) {
    if (!utility::find_in(c.research, players[id].research_level.available())) {
      throw player_error("Invalid research choice submitted by player " + to_string(id) + ": " + c.research);
    }
  }
  players[id].research_level.researching = c.research;

  // solar choices: require research to be applied
  for (auto &x : c.solar_choices){
    // validate
    auto e = get_entity(x.first);
    
    if (e -> owner != id){
      throw player_error("validate_choice: error: solar choice by player " + to_string(id) + " for " + x.first + " owned by " + to_string(e -> owner));
    }
    
    if (!e -> isa(solar::class_id)){
      throw player_error("validate_choice: error: solar choice by player " + to_string(id) + " for " + x.first + ": not a solar!");
    }

    if (x.second.do_develop()){
      list<string> av = get_solar(x.first) -> available_facilities(players[id].research_level);
      for (auto i = x.second.building_queue.begin(); i != x.second.building_queue.end(); i++) {
	if (!utility::find_in(*i, av)) {
	  // maybe a dependency got bombed or something...
	  x.second.building_queue.erase(i--);
	}
      }
    }

    if (!utility::find_in(x.second.governor, keywords::governor)) {
      throw player_error("validate_choice: invalid governor: " + x.second.governor);
    }

    // apply
    solar::ptr s = get_solar(x.first);
    if (!(s -> choice_data.do_produce() && x.second.do_produce() && s -> choice_data.ship_queue.front() == x.second.ship_queue.front())) {
      s -> ship_progress = 0;
    }
    s -> choice_data = x.second;
  }

  // pass military choice to player object
  players[id].military = c.military;

  // commands: validate
  for (auto x : c.commands) {
    auto e = get_entity(x.first);
    if (e -> owner != id) {
      throw player_error("validate_choice: error: command by player " + to_string(id) + " for " + x.first + " owned by " + to_string(e -> owner));
    }
  }

  // apply
  for (auto x : c.commands){
    commandable_object::ptr v = utility::guaranteed_cast<commandable_object>(get_entity(x.first));
    v -> give_commands(x.second, this);
  }
}

void game_data::add_entity(game_object::ptr p){
  if (entity.count(p -> id)) throw logical_error("add_entity: already exists: " + p -> id);  
  entity[p -> id] = p;
  p -> on_add(this);
}

void game_data::remove_entity(combid i){
  if (!entity.count(i)) throw logical_error("remove_entity: " + i + ": doesn't exist!");
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
  float density = 0.1;
  float area = sh.size() / density;
  float radius = sqrt(area / M_PI);
  
  for (auto x : sh){
    ship::ptr s = get_ship(x);
    s -> position.x = utility::random_normal(p.x, radius);
    s -> position.y = utility::random_normal(p.y, radius);
    entity_grid -> insert(s -> id, s -> position);
  }
}

point game_data::terrain_forcing(point p) {
  list<idtype> tids = terrain_at(p, 10);
  if (tids.size()) {
    terrain_object x = terrain[tids.front()];
    int j = x.triangle(p, 10);
    if (j > -1) {
      float test = utility::triangle_relative_distance(x.center, x.get_vertice(j), x.get_vertice(j+1), p, 10);
      if (test > -1) {
	if (test < 1) {
	  return (1 - test) * (p - x.center);
	}
      } else {
	throw logical_error("Forcing triangle without relative distance value!");
      }
    }
  }

  return point(0,0);
}

void game_data::extend_universe(int i, int j, bool starting_area) {
  pair<int, int> idx = make_pair(i, j);
  if (discovered_universe.count(idx)) return;  
  discovered_universe.insert(idx);

  float ratio = settings.space_index_ratio; 
  point ul(i * ratio, j * ratio);
  point br((i + 1) * ratio, (j + 1) * ratio);
  point center = utility::scale_point(ul + br, 0.5);
  float distance = utility::l2norm(center);
  float bounty = exp(-pow(distance / settings.galaxy_radius, 2));
  bounty = utility::linsig(utility::random_normal(bounty, 0.2 * bounty));
  float nbuf = bounty * pow(ratio, 2) * settings.solar_density;
  int n_solar = fmax(utility::random_normal(nbuf, 0.2 * nbuf), 0);
  float var = utility::random_uniform(0.3, 2);
    
  if (starting_area) {
    n_solar = 10;
    bounty = 1;
    var = 0.3;
  }

  if (n_solar == 0) return;

  // select positions
  vector<point> static_pos;    
  for (auto test : entity_grid -> search(center, ratio)) {
    if (identifier::get_type(test.first) == solar::class_id) static_pos.push_back(test.second);
  }

  vector<point> x(n_solar);
  vector<point> v(n_solar, point(0, 0));
    
  auto point_init = [ul, br] () {
    return point(utility::random_uniform(ul.x, br.x), utility::random_uniform(ul.y, br.y));
  };
    
  for (auto &y : x) y = point_init();

  if (n_solar > 1) {
    // shake positions so solars don't end up on top of each other
    vector<point> all_points;

    for (float e = 10; e >= 1; e *= 0.95) {
      all_points = x;
      all_points.insert(all_points.end(), static_pos.begin(), static_pos.end());

      for (int i = 0; i < n_solar; i++) {
	// check against other solars
	for (int j = i + 1; j < all_points.size(); j++) {
	  point d = x[i] - all_points[j];
	  if (utility::l2norm(d) < 50) {
	    x[i] += utility::normalize_and_scale(d, e);
	  }
	}

	// check against terrain
	list<idtype> tids = terrain_at(x[i], 50);
	for (auto tid : tids) {
	  if (terrain[tid].triangle(x[i], 50) > -1) {
	    x[i] += utility::normalize_and_scale(x[i] - terrain[tid].center, e);
	  }
	}	
      }
    }
  }

  // make solars
  for (auto p : x) add_entity(solar::create(p, bounty, var));

  // add impassable terrain
  static int terrain_idc = 0;
  static mutex m;
  
  float min_length = 10;
  auto avoid_point = [this, min_length] (terrain_object &obj, point p, float rad) -> bool {
    auto j = obj.triangle(p, rad);
    if (j > -1) {
      float test = utility::triangle_relative_distance(obj.center, obj.get_vertice(j), obj.get_vertice(j+1), p, rad);
      if (test > -1 && test < 1) {
	point d1 = obj.get_vertice(j) - obj.center;
	point d2 = obj.get_vertice(j + 1) - obj.center;
	float shortest = fmin(utility::l2norm(d1), utility::l2norm(d2));
	if (test * shortest <= min_length) {
	  return false;
	}
	obj.set_vertice(j, obj.center + test * d1);
	obj.set_vertice(j + 1, obj.center + test * d2);
      }
    }
    return true;
  };

  list<solar::ptr> all_solars = all<solar>();
  int n_terrain = utility::random_int(4);
  float min_dist = 40;
  for (int i = 0; i < n_terrain; i++) {
    terrain_object obj;

    // select center sufficiently far from any solars
    bool passed = false;
    int count = 0;
    while (count++ < 100 && !passed) {
      obj.center = point_init();
      passed = true;
      for (auto p : all_solars) passed &= utility::l2norm(obj.center - p -> position) > p -> radius;
      for (auto p : terrain) passed &= utility::l2norm(obj.center - p.second.center) > min_dist + 2 * min_length;
    }

    if (!passed) continue;

    // adapt other terrain to not cover the center
    for (auto &x : terrain) if (!avoid_point(x.second, obj.center, min_dist + 2 * min_length)) continue;

    // generate random border
    for (float angle = 0; angle < 2 * M_PI - 0.1; angle += utility::random_uniform(0.2, 0.5)) {
      float rad = fmax(utility::random_normal(120, 20), min_length);
      obj.border.push_back(obj.center + rad * utility::normv(angle));
    }

    // go through solars and make sure we don't cover them
    for (auto p : all_solars) if (!avoid_point(obj, p -> position, p -> radius)) continue;

    // go through terrain so we don't overlap
    bool failed = false;
    for (auto &x : terrain) {
      pair<int,int> test;
      while ((test = obj.intersects_with(x.second)).first > -1) {
	int i = test.first;
	point d1 = obj.get_vertice(i) - obj.center;
	point d2 = obj.get_vertice(i+1) - obj.center;

	obj.set_vertice(i, obj.center + utility::normalize_and_scale(d1, 0.9 * utility::l2norm(d1)));
	obj.set_vertice(i+1, obj.center + utility::normalize_and_scale(d2, 0.9 * utility::l2norm(d2)));
	if (fmin(utility::l2norm(d1), utility::l2norm(d2)) < min_length) {
	  failed = true;
	  break;
	}
      }

      if (!failed) {
	for (auto y : x.second.border) if (!avoid_point(obj, y, min_dist)) continue;
	for (auto y : obj.border) if (!avoid_point(x.second, y, min_dist)) continue;
	auto test = obj.intersects_with(x.second);
	if (test.first > -1) {
	  server::log("add terrain: avoid point caused intersection!", "warning");
	  failed = true;
	  break;
	}
      } else {
	break;
      }
    }

    if (failed) continue;

    // safely generate terrain object id
    int tid;
    m.lock();
    tid = terrain_idc++;
    m.unlock();
    terrain[tid] = obj;
  }

  // finally check all terrain
  for (auto x : terrain) {
    for (auto y : terrain) {
      if (x.first == y.first) continue;
      if (x.second.intersects_with(y.second).first > -1) {
	throw logical_error("Generated intersecting terrain!");
      }
    }
  }

  // check waypoints so they aren't covered
  for (auto w : all<waypoint>()) {
    list<idtype> tids = terrain_at(w -> position, 0);
    for (auto tid : tids) {
      if (terrain[tid].triangle(w -> position, 0) > -1) {
	remove_entity(w -> id);
	break;
      }
    }
  }
}

void game_data::discover(point x, float r, bool starting_area) {
  float ratio = settings.space_index_ratio; 
  auto point2idx = [ratio] (float u) {return floor(u / ratio);};
  int x1 = point2idx(x.x - r);
  int y1 = point2idx(x.y - r);
  int x2 = point2idx(x.x + r);
  int y2 = point2idx(x.y + r);

  for (int i = x1; i <= x2; i++) {
    for (int j = y1; j <= y2; j++) {
      extend_universe(i, j, starting_area);
    }
  }
}

void game_data::update_discover() {
  for (auto e : all<physical_object>()) if (e -> is_active()) discover(e -> position, e -> vision());
}

void game_data::rebuild_evm() {
  evm.clear();

  // rebuild vision matrix
  for (auto x : entity) {
    if (x.second -> is_active() && x.second -> owner != game_object::neutral_owner) {
      // entity sees itself
      evm[x.second -> owner].insert(x.first);

      // only physical entities can see others
      if (x.second -> isa(physical_object::class_id)) {
	physical_object::ptr e = utility::guaranteed_cast<physical_object>(x.second);
	for (auto i : entity_grid -> search(e -> position, e -> vision())) {
	  if (i.first == x.first) continue;
	  
	  game_object::ptr t = get_entity(i.first);
	  // only see other players' entities if they are physical and active
	  if (t -> isa(physical_object::class_id) && t -> is_active() && e -> can_see(t)) {
	    evm[e -> owner].insert(t -> id);
	  }
	}
      }
    }
  }
}

solar::ptr game_data::closest_solar(point p, idtype id) {
  solar::ptr s = 0;

  try {
    s = utility::value_min(all<solar>(), (function<float(solar::ptr)>) [this, p, id] (solar::ptr t) -> float {
	bool owned = t -> owner == id || (id == game_object::any_owner && t -> owner >= 0);
	if (owned) {
	  return utility::l2d2(t -> position - p);
	}else{
	  return INFINITY;
	}
      });
  } catch (exception &e) {
    // player doesn't own any solars
    return 0;
  }

  return s;
}

void game_data::increment(){
  // clear frame
  remove_entities.clear();
  interaction_buffer.clear();
  for (auto &p : players) {
    p.second.animations.clear();
    p.second.log.clear();
  }
  update_discover();
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

  // // perform collisions
  // for (auto x : collision_buffer) collide_ships(x);

  // remove units twice, since removing ships can cause fleets to set
  // the remove flag
  remove_units();
  remove_units();
  
  for (auto x : entity) if (x.second -> is_active()) x.second -> post_phase(this);
}

void game_data::build_players(list<server::client_t*> clients){
  // build player data
  vector<sint> colbuf = utility::different_colors(clients.size());
  int i = 0;
  player p;
  for (auto x : clients){
    p.name = x -> name;
    p.color = colbuf[i++];
    players[x -> id] = p;
  }
}

// players and settings should be set before build is called
void game_data::build(){
  static research::data rbase;
  
  if (players.empty()) throw classified_error("game_data: build: no players!", "error");

  auto make_home_solar = [this] (point p, idtype pid) {
    cost::res_t initial_resources;
    for (auto v : keywords::resource) initial_resources[v] = 1000;

    solar::ptr s = solar::create(p, 1);
    s -> owner = pid;
    s -> was_discovered = true;
    s -> available_resource = initial_resources;
    s -> water = 1000;
    s -> space = 1000;
    s -> population = 1000;
    s -> happiness = 1;
    s -> ecology = 1;
    s -> radius = settings.solar_meanrad;
    s -> choice_data.allocation = cost::sector_allocation::base_allocation();
    s -> choice_data.governor = keywords::key_culture;
    s -> facility_access("shipyard") -> level = 1;
    s -> facility_access("research facility") -> level = 1;
    s -> facility_access("missile turret") -> level = 1;

    // debug: start with some ships
    hm_t<string, int> starter_fleet;
    if (settings.starting_fleet == "single") {
      starter_fleet["fighter"] = 1;
    } else if (settings.starting_fleet == "voyagers") {
      starter_fleet["voyager"] = 2;
    } else if (settings.starting_fleet == "battleships") {
      starter_fleet["battleship"] = 40;
    } else if (settings.starting_fleet == "massive") {
      for (auto sc : ship::all_classes()) starter_fleet[sc] = 100;
    } else {
      throw player_error("Invalid starting fleet option: " + settings.starting_fleet);
    }
    
    for (auto sc : starter_fleet) {
      for (int j = 0; j < sc.second; j++) {
	ship sh = rbase.build_ship(sc.first, s);
	if ((!sh.depends_tech.empty()) && settings.starting_fleet == "massive") {
	  sh.upgrades += research::data::get_tech_upgrades(sh.ship_class, sh.depends_tech);
	}
	sh.is_landed = true;
	sh.owner = pid;
	s -> ships.insert(sh.id);
	add_entity(ship::ptr(new ship(sh)));
      }
    }
    add_entity(s);
    discover(s -> position, 0, true);
  };

  allocate_grid();
  float angle = utility::random_uniform(0, 2 * M_PI);
  float np = players.size();
  for (auto &p : players) {
    point p_base = utility::scale_point(utility::normv(angle), settings.galaxy_radius);
    point p_start = utility::random_point_polar(p_base, 0.2 * settings.galaxy_radius);
    make_home_solar(p_start, p.first);
    angle += 2 * M_PI / np;
  }

  // generate universe between players
  int idx_max = floor(settings.galaxy_radius / settings.space_index_ratio);
  for (int i = -idx_max; i <= idx_max; i++) {
    for (int j = -idx_max; j <= idx_max; j++) {
      extend_universe(i, j);
    }
  }
}

// clean up things that will be reloaded from client
void game_data::pre_step(){
  // clear waypoints and fleets, but don't list removals as client
  // manages them
  for (auto i : all<waypoint>()) i -> remove = true;
  for (auto i : all<fleet>()) i -> remove = true;
  remove_units();
  remove_entities.clear();

  // force grid to rebuild
  rehash_grid();

  // update research facility level for use in validate choice
  update_research_facility_level();
}

void game_data::update_research_facility_level() {
  hm_t<idtype, hm_t<string, int> > level;
  for (auto i : all<solar>()){
    if (i -> owner > -1){
      for (auto &f : i -> development) {
	level[i -> owner][f.first] = max(level[i -> owner][f.first], f.second.level);
      }
    }
  }

  for (auto &x : players) {
    x.second.research_level.facility_level = level[x.first];
  }
}

// pool research and remove unused waypoints
void game_data::end_step(){
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
  }

  // clear log
  for (auto &x : players) x.second.log.clear();

  // pool research
  update_research_facility_level();
  
  hm_t<idtype, float> pool;
  for (auto i : all<solar>()){
    if (i -> owner > -1){
      pool[i -> owner] += i -> research_points;
      i -> research_points = 0;
    }
  }

  for (auto x : players) {
    idtype id = x.first;
    research::data &r = players[id].research_level;
    
    // apply
    if (r.researching.length() > 0) {
      if (utility::find_in(r.researching, r.available())) {
	research::tech &t = r.access(r.researching);
	t.progress += pool[id];
	if (t.progress >= t.cost_time) {
	  t.progress = 0;
	  t.level = 1;
	  players[id].log.push_back("Researched " + r.researching);
	  r.researching = "";
	}
      } else {
	players[id].log.push_back("Can't research " + r.researching);
	r.researching = "";
      }
    }
  }
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

animation_tracker_info game_data::get_tracker(combid id) {
  animation_tracker_info info;
  info.eid = id;

  if (entity.count(id)) {
    info.p = get_entity(id) -> position;

    if (get_entity(id) -> isa(ship::class_id)) {
      ship::ptr s = get_ship(id);
      info.v = s -> stats[sskey::key::speed] * utility::normv(s -> angle);
    } else {
      info.v = point(0, 0);
    }
  }

  return info;
}

void game_data::log_bombard(combid a, combid b) {
  ship::ptr s = get_ship(a);
  solar::ptr t = get_solar(b);
  int delay = utility::random_int(sub_frames);

  animation_data x;
  x.t1 = get_tracker(s -> id);
  x.t2 = get_tracker(t -> id);
  x.color = players[s -> owner].color;
  x.cat = animation_data::category::bomb;
  x.magnitude = s -> stats[sskey::key::solar_damage];
  x.delay = delay;
  
  animation_data sh;
  float shield = t -> compute_shield_power();
  sh.t1 = get_tracker(t -> id);
  sh.radius = 1.2 * t -> radius;
  sh.magnitude = shield;
  sh.color = players[t -> owner].color;
  sh.cat = animation_data::category::shield;
  sh.delay = delay;
  
  for (auto &p : players) {
    if (evm[p.first].count(b)) {
      p.second.animations.push_back(x);
      if (shield > 0) p.second.animations.push_back(sh);
    }
  }
}

void game_data::log_ship_fire(combid a, combid b) {
  game_object::ptr s = get_entity(a);
  ship::ptr t = get_ship(b);
  int delay = utility::random_int(sub_frames);

  animation_data x;
  x.t1 = get_tracker(s -> id);
  x.t2 = get_tracker(t -> id);
  x.color = players[s -> owner].color;
  x.cat = animation_data::category::shot;
  x.delay = delay;
  
  if (s -> isa(ship::class_id)) {
    ship::ptr sp = utility::guaranteed_cast<ship>(s);
    x.magnitude = sp -> stats[sskey::key::ship_damage];
  } else {
    x.magnitude = 1;
  }

  animation_data sh;
  sh.t1 = get_tracker(t -> id);
  sh.magnitude = t -> stats[sskey::key::shield];
  sh.radius = 1.2 * t -> radius;
  sh.color = players[t -> owner].color;
  sh.cat = animation_data::category::shield;
  sh.delay = delay;

  for (auto &p : players) {
    if (evm[p.first].count(a) || evm[p.first].count(b)) {
      p.second.animations.push_back(x);
    }

    if (evm[p.first].count(b) && t -> stats[sskey::key::shield] > 0) {
      p.second.animations.push_back(sh);
    }
  }  
}

void game_data::log_ship_destroyed(combid a, combid b) {
  game_object::ptr s = get_entity(a);
  ship::ptr t = get_ship(b);
  int delay = utility::random_int(sub_frames);

  // text log
  string self_identifier = s -> isa(ship::class_id) ? get_ship(a) -> ship_class : "solar";
  string log_p1 = "Your " + self_identifier + " destroyed an enemy " + t -> ship_class;
  string log_p2 = "Your " + t -> ship_class + " was shot down by an enemy " + self_identifier;
  
  players[s -> owner].log.push_back(log_p1);
  players[t -> owner].log.push_back(log_p2);

  // animations
  animation_data x;
  x.t1 = get_tracker(t -> id);
  x.magnitude = t -> stats[sskey::key::mass];
  x.color = players[t -> owner].color;
  x.cat = animation_data::category::explosion;
  x.delay = delay;

  for (auto &p : players) {
    if (evm[p.first].count(b)) {
      p.second.animations.push_back(x);
    }
  }
}

void game_data::log_message(combid a, string v_full, string v_short) {
  ship::ptr s = get_ship(a);

  players[s -> owner].log.push_back(v_full);

  animation_data x;
  x.t1 = get_tracker(s -> id);
  x.cat = animation_data::category::message;
  x.text = v_short;
  x.color = players[s -> owner].color;
  x.delay = 0;
  players[s -> owner].animations.push_back(x);
}

float game_data::get_dt() {
  return sub_frames * settings.dt;
}


game_object::ptr entity_package::get_entity(combid i){
  if (entity.count(i)){
    return entity[i];
  }else{
    throw logical_error("game_data::get_entity: not found: " + i);
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
  if (entity.size()) throw logical_error("Attempting to assign game_data: has entities!");

  for (auto x : g.entity) entity[x.first] = x.second -> clone();
  players = g.players;
  settings = g.settings;
  remove_entities = g.remove_entities;
  terrain = g.terrain;
  evm = g.evm;
}

void entity_package::clear_entities(){
  for (auto x : entity) delete x.second;
  entity.clear();
}

