#include <iostream>

#include "game_data.h"
#include "ship.h"
#include "fleet.h"
#include "utility.h"
#include "upgrades.h"
#include "serialization.h"

using namespace std;
using namespace st3;

const string ship::class_id = "ship";
string ship::starting_ship;

const hm_t<string, ship_stats>& ship_stats::table(){
  static bool init = false;
  static hm_t<string, ship_stats> buf;

  if (init) return buf;

  auto pdoc = utility::get_json("ship");
  auto &doc = (*pdoc)["ship"];

  ship_stats s, a;
  bool success;
  s.stats[ssfloat_t::key::speed] = 0.5;
  s.stats[ssfloat_t::key::vision_range] = 50;
  s.stats[ssfloat_t::key::hp] = 1;
  s.stats[ssfloat_t::key::interaction_radius_value] = 20;
  s.stats[ssfloat_t::key::load_time] = 50;
  s.upgrades.insert(interaction::land);

  // read ships from json structure
  for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); i++) {
    a = s;
    a.ship_class = i -> name.GetString();

    for (auto j = i -> value.MemberBegin(); j != i -> value.MemberEnd(); j++) {
      success = false;
      string name = j -> name.GetString();
      if (j -> value.IsDouble()) {
	float value = j -> value.GetDouble();
	success |= a.insert(name, value);

	if (name == "is starting ship") {
	  ship::starting_ship = a.ship_class;
	  success = true;
	} else if (name == "depends facility level") {
	  a.depends_facility_level = value;
	  success = true;
	}
      }else if (j -> value.IsString()) {
	if (name == "depends tech") {
	  a.depends_tech = j -> value.GetString();
	  success = true;
	}
      }else if (j -> value.IsArray()) {
	if (name == "upgrades") {
	  for (auto u = j -> value.Begin(); u != j -> value.End(); u++) a.upgrades.insert(u -> GetString());
	  success = true;
	} else if (name == "shape") {
	  pair<point, unsigned char> v;
	  for (auto j = i -> value.Begin(); j != i -> value.End(); j++) {
	    v.first.x = (*j)["x"].GetDouble();
	    v.first.y = (*j)["y"].GetDouble();
	    v.second = (*j)["c"].GetString()[0];
	    a.shape.push_back(v);
	  }
	  success = true;
	} else if (name == "tags") {
	  for (auto j = i -> value.Begin(); j != i -> value.End(); j++) a.tags.insert(j -> GetString());
	  success = true;
	}
      }else if (j -> value.IsObject()) {
	if (name == "cost") {
	  for (auto k = j -> value.MemberBegin(); k != j -> value.MemberEnd(); k++) {
	    string res_name = k -> name.GetString();
	    bool sub_success = false;
	    for (auto r : keywords::resource) {
	      if (res_name == r) {
		a.build_cost[r] = k -> value.GetDouble();
		sub_success = true;
	      }
	    }
	    if (name == "time") {
	      a.build_time = k -> value.GetDouble();
	      sub_success = true;
	    }

	    if (!sub_success) {
	      throw runtime_error("Invalid cost specification for ship " + a.ship_class + " in: " + res_name);
	    }
	  }
	  success = true;
	}
      }

      if (!success) {
	throw runtime_error("Invalid ship term: " + name);
      }
    }
    
    buf[a.ship_class] = a;
  }

  delete pdoc;
  init = true;
  return buf;
}

ship::ship(){}

ship::ship(const ship &s) {
  copy_from(s);
}

ship::ship(const ship_stats &s) : ship_stats(s), physical_object() {
  base_stats = s;
  fleet_id = identifier::source_none;
  remove = false;
  load = 0;
  passengers = 0;
  is_landed = false;
}

ship::~ship(){}

list<string> ship::all_classes() {
  return utility::get_map_keys(table());
}

void ship::set_stats(ship_stats s){
  static_cast<ship_stats&>(*this) = s;
}

void ship::pre_phase(game_data *g){
  // load stuff
  load = fmin(load + 1, base_stats.stats[key::load_time]);
  stats[key::hp] = fmin(stats[key::hp] + stats[key::regeneration], base_stats.stats[key::hp]);
  stats[key::shield] = fmin(stats[key::shield] + 0.01, base_stats.stats[key::shield]);
}

// TODO: rewrite using fleet policies
void ship::move(game_data *g){
  if (!has_fleet()) return;
  fleet::ptr f = g -> get_fleet(fleet_id);
  fleet::suggestion suggest = f -> suggest(id, g);

  bool summon = suggest.id & fleet::suggestion::summon;
  bool engage = suggest.id & fleet::suggestion::engage;
  bool scatter = suggest.id & fleet::suggestion::scatter;
  bool travel = suggest.id & fleet::suggestion::travel;
  bool activate = suggest.id & fleet::suggestion::activate;
  bool hold = suggest.id & fleet::suggestion::hold;

  point target_position = f -> stats.target_position;
  float fleet_target_angle = utility::point_angle(target_position - f -> position);
  point fleet_delta = f -> position - position;
  float fleet_angle = utility::point_angle(fleet_delta);
  float target_angle = fleet_target_angle;
  point target_delta = target_position - position;

  // use the local temporary target when available
  if (engage || scatter || travel || activate) {
    target_position = suggest.p;
    target_delta = target_position - position;
    target_angle = utility::point_angle(target_delta);
  }

  float target_speed = f -> speed_limit;

  float nrad = interaction_radius();
  list<combid> neighbours = g -> search_targets(id, position, nrad, target_condition(target_condition::owned, ship::class_id).owned_by(owner));
  list<combid> local_enemies = g -> search_targets(id, position, interaction_radius(), target_condition(target_condition::enemy, ship::class_id).owned_by(owner));

  if (summon) target_angle = fleet_angle;

  if (summon && travel) {
    float test = utility::angle_difference(fleet_target_angle, fleet_angle);
    if (test > 2 * M_PI / 3) {
      // in front of fleet
      if (check_space(fleet_target_angle + M_PI)) {
	// slow down
	target_speed = 0.8 * f -> speed_limit;
      }
    } else if (test > M_PI / 3) {
      // side of fleet
      if (check_space(fleet_angle)) {
	target_angle += 0.1 * utility::angle_difference(fleet_angle, target_angle);
      }
    } else {
      // back of fleet
      if (check_space(fleet_target_angle)) {
	target_speed = stats[key::speed];
      }
    }
  }

  if (engage) {
    combid target_id;
    float best = -1;

    for (auto sid : local_enemies) {
      point delta = g -> get_ship(sid) -> position - position;
      float value = cos(utility::angle_difference(utility::point_angle(delta), angle));
      if (value > best) {
	best = value;
	target_id = sid;
      }
    }

    if (best > 0.7) {
      // I've got a shot!
      interaction_info info;
      info.source = id;
      info.target = target_id;
      info.interaction = interaction::space_combat;
      g -> interaction_buffer.push_back(info);
    }

    if (utility::l2norm(target_delta) > 10) {
      target_speed = stats[key::speed];
    } else {
      target_speed = 0;
    }
  }

  if (scatter) {
    // todo
  }

  if (hold) {
    target_speed = 0;
  }

  if (activate) {
    if (f -> com.target != identifier::target_idle && compile_interactions().count(f -> com.action)) {
      game_object::ptr e = get_entity(f -> com.target);
      if (can_see(e) && utility::l2norm(e -> position - position) <= interaction_radius()) {
	interaction_info info;
	info.source = id;
	info.target = e -> id;
	info.interaction = f -> com.action;
	g -> interaction_buffer.push_back(info);
      }
    }
  }

  // todo
  float angle_increment = 0.1;
  float epsilon = 0.01;
  float angle_sign = utility::signum(utility::angle_difference(target_angle, angle), epsilon);

  angle += angle_increment * angle_sign;
  position = position + utility::scale_point(utility::normv(angle), f -> speed_limit);

  g -> entity_grid -> move(id, position);
}

void ship::post_phase(game_data *g){}

void ship::receive_damage(game_object::ptr from, float damage) {
  damage -= stats[key::shield];
  stats[key::shield] = fmax(stats[key::shield] - 0.1 * damage, 0);
  stats[key::hp] -= damage;
  remove = stats[key::hp] <= 0;
  cout << "ship::receive_damage: " << id << " takes " << damage << " damage from " << from -> id << " - remove = " << remove << endl;
}

void ship::on_remove(game_data *g){
  if (g -> entity.count(fleet_id)) {
    g -> get_fleet(fleet_id) -> remove_ship(id);
  }
  game_object::on_remove(g);
}

ship::ptr ship::create(){
  return ptr(new ship());
}

ship::ptr ship::clone(){
  return dynamic_cast<ship::ptr>(clone_impl());
}

game_object::ptr ship::clone_impl(){
  return ptr(new ship(*this));
}

bool ship::serialize(sf::Packet &p){
  return p << class_id << *this;
}

float ship::vision(){
  return stats[key::vision_range];
}

set<string> ship::compile_interactions(){
  set<string> res;
  auto utab = upgrade::table();
  for (auto v : upgrades) res += utab[v].inter;
  return res;
}

list<combid> ship::confirm_interaction(string a, list<combid> targets, game_data *g) {
  list<combid> allowed;
  if (has_fleet()){
    for (auto t : targets) {
      if (g -> get_fleet(fleet_id) -> confirm_ship_interaction(a, t)){
	allowed.push_back(t);
      }
    }
  }else{
    if (a == interaction::space_combat) {
      allowed = targets;
    }
  }

  // chose at most one target
  list<combid> result;
  if (a == "hive support"){
    result = allowed;
  }else{
    if (!allowed.empty()) result.push_back(utility::uniform_sample(allowed));
  }
  
  return result;
}

bool ship::accuracy_check(float a, ship::ptr t) {
  return utility::random_uniform() < (cos(a) + 1) / 2;
}

float ship::interaction_radius() {
  return interaction_radius_value;
}

bool ship::is_active(){
  return !is_landed;
}

bool ship::has_fleet() {
  return fleet_id != identifier::source_none;
}

void ship::on_liftoff(solar::ptr from, game_data *g){
  g -> players[owner].research_level.repair_ship(*this, from);
  is_landed = false;

  for (auto v : upgrades) {
    upgrade u = upgrade::table().at(v);
    for (auto i : u.on_liftoff) interaction::table().at(i).perform(this, from, g);
  }
}

void ship::copy_from(const ship &s){
  (*this) = s;
}

bool ship::isa(string c) {
  return c == ship::class_id || c == physical_object::class_id;
}

bool ship::can_see(game_object::ptr x) {
  float r = 1;

  if (x -> isa(ship::class_id)) {
    ship::ptr s = utility::guaranteed_cast<ship>(x);
    r = vision() * fmin((detection + 1) / (s -> stealth + 1), 1);
  }
  
  float d = utility::l2norm(x -> position - position);
  return d < r;
}

// ship stats stuff

ship_stats_modifier::ship_stats_modifier() {
  a = 1;
  b = 0;
}

float ship_stats_modifier::apply(float x) {
  return a * x + b;
}

void ship_stats_modifier::combine(const ship_stats_modifier &x) {
  a *= x.a;
  b += x.b;
}

template<typename T>
modifiable_ship_stats::modifiable_ship_stats() {
  stats.resize(key::count);
}

void ssmod_t::combine(const ssmod_t &b) {
  for (int i = 0; i < key::count; i++) {
    stats[i] += b.stats[i];
  }
}

void ship_stats::modify_with(const ssmod_t &b) {
  for (int i = 0; i < key::count; i++) {
    stats[i] = b.stats[i].apply(stats[i]);
  }
}

ssfloat_t::ssfloat_t() : modifiable_ship_stats<sfloat>(){
  for (auto &x : stats) x = 0;
}

ship_stats::ship_stats() : ssfloat_t(){
  depends_facility_level = 0;
  build_time = 0;
}
