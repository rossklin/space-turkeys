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
  s.stats[ssfloat_t::key::mass] = 1;
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
      if (j -> value.IsNumber()) {
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
  radius = pow(stats[key::mass], 2/(float)3);
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
  bool local = engage || scatter || activate;

  float fleet_target_angle = utility::point_angle(f -> stats.target_position - f -> position);
  point fleet_delta = f -> position - position;
  float fleet_angle = utility::point_angle(fleet_delta);
  float target_angle = fleet_target_angle;
  float target_speed = f -> stats.speed_limit;

  float nrad = interaction_radius();
  list<combid> neighbours = g -> search_targets(id, position, nrad, target_condition(target_condition::any_alignment, ship::class_id).owned_by(owner));

  auto check_space = [this, neighbours, g] (float a) -> bool {
    float spread = 0.2 * M_PI;
    float buffer = 2 * radius;

    for (combid sid : neighbours) {
      ship::ptr s = g -> get_ship(sid);
      point delta = s -> position - position;
      if (utility::l2norm(delta) < radius + s -> radius + buffer) {
	if (abs(utility::angle_difference(utility::point_angle(delta), 0)) < spread) return false;
      }
    }

    return true;
  };

  // angle and speed for summon
  auto compute_summon = [=] (float &a, float &s) {
    if (travel) {
      float test = utility::angle_difference(fleet_target_angle, fleet_angle);
      if (test > 2 * M_PI / 3) {
	// in front of fleet
	if (check_space(fleet_target_angle + M_PI)) {
	  // slow down
	  s = 0.8 * f -> stats.speed_limit;
	}
      } else if (test > M_PI / 3) {
	// side of fleet
	if (check_space(fleet_angle)) {
	  a = fleet_target_angle + 0.1 * utility::angle_difference(fleet_angle, fleet_target_angle);
	}
      } else {
	// back of fleet
	if (check_space(fleet_target_angle)) {
	  s = stats[key::speed];
	}
      }
    } else {
      a = fleet_angle;
    }
  };

  // angle and speed when running local policies
  auto compute_local = [this, suggest] (float &a, float &s) {
    point local_delta = suggest.p - position;
    a = utility::point_angle(local_delta);

    if (utility::l2norm(local_delta) > 10) {
      s = stats[key::speed];
    } else {
      s = 0;
    }
  };

  // compute desired angle and speed
  if (summon) {
    compute_summon(target_angle, target_speed);
  } else if (local) {
    compute_local(target_angle, target_speed);
  } else if (hold) {
    target_speed = 0;
  } else if (travel) {
    // try to follow ships in front of you
    for (auto sid : neighbours) {
      ship::ptr s = g -> get_ship(sid);
      if (s -> owner == owner) {
	float weight = (cos(utility::angle_difference(utility::point_angle(s -> position - position), angle)) + 1) / 2;
	target_angle += 0.1 * weight * utility::angle_difference(target_angle, s -> angle);
      }
    }
  }

  // shoot enemies if able
  if (engage) {
    combid target_id;
    float best = -1;

    for (auto sid : neighbours) {
      ship::ptr s = g -> get_ship(sid);
      if (s -> owner == owner) continue;
      
      point delta = s -> position - position;
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
  }

  // activate fleet command action if able
  if (activate) {
    if (f -> com.target != identifier::target_idle && compile_interactions().count(f -> com.action)) {
      game_object::ptr e = g -> get_entity(f -> com.target);
      if (can_see(e) && utility::l2norm(e -> position - position) <= interaction_radius()) {
	interaction_info info;
	info.source = id;
	info.target = e -> id;
	info.interaction = f -> com.action;
	g -> interaction_buffer.push_back(info);
      }
    }
  }

  // activate hive support on all friendly neighbours if able
  if (compile_interactions().count(interaction::hive_support)) {
    for (auto sid : neighbours) {
      ship::ptr s = g -> get_ship(sid);
      if (s -> owner == owner) {
	interaction_info info;
	info.source = id;
	info.target = s -> id;
	info.interaction = interaction::hive_support;
	g -> interaction_buffer.push_back(info);
      }
    }
  }

  // check if there is free space at target angle
  if (!check_space(target_angle)) {
    float check_first = 1 - 2 * utility::random_int(2);
    float spread = 0.2 * M_PI;
    
    if (check_space(target_angle + check_first * spread)) {
      target_angle = target_angle + check_first * spread;
    } else {
      check_first *= -1;
      if (check_space(target_angle + check_first * spread)) {
	target_angle = target_angle + check_first * spread;
      } else {
	target_speed = 0;
      }
    }
  }

  // check unit collisions
  for (auto sid : neighbours) {
    ship::ptr s = g -> get_ship(sid);
    point delta = s -> position - position;
    if (utility::l2norm(delta) < radius + s -> radius) g -> collision_buffer.insert(id_pair(id, sid));
  }

  float angle_increment = fmin(0.1 / stats[key::mass], 0.5);
  float acceleration = 0.1 * base_stats.stats[key::speed] / stats[key::mass];
  float epsilon = 0.01;
  float angle_miss = utility::angle_difference(target_angle, angle);
  float angle_sign = utility::signum(angle_miss, epsilon);

  // slow down if missing angle
  target_speed = (cos(angle_miss) + 1) / 2 * target_speed;
  float speed_miss = target_speed - stats[key::speed];
  float speed_sign = utility::signum(speed_miss, epsilon);
  stats[key::speed] += fmin(acceleration, abs(speed_miss)) * speed_sign;
  angle += fmin(angle_increment, abs(angle_miss)) * angle_sign;
  position = position + utility::scale_point(utility::normv(angle), stats[key::speed]);

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
  if (a == interaction::hive_support){
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
  return radius + interaction_radius_value;
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
  if (!x -> is_active()) return false;

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

float ship_stats_modifier::apply(float x) const{
  return a * x + b;
}

void ship_stats_modifier::combine(const ship_stats_modifier &x) {
  a *= x.a;
  b += x.b;
}

template<typename T>
modifiable_ship_stats<T>::modifiable_ship_stats() {
  stats.resize(key::count);
}

template<typename T>
modifiable_ship_stats<T>::modifiable_ship_stats(const modifiable_ship_stats<T> &s) {
  stats = s.stats;
}

void ssmod_t::combine(const ssmod_t &b) {
  for (int i = 0; i < key::count; i++) {
    stats[i].combine(b.stats[i]);
  }
}

void ship_stats::modify_with(const ssmod_t &b) {
  for (int i = 0; i < key::count; i++) {
    stats[i] = b.stats[i].apply(stats[i]);
  }
}

ssmod_t::ssmod_t() : modifiable_ship_stats<ship_stats_modifier>(){}
ssmod_t::ssmod_t(const ssmod_t &s) : modifiable_ship_stats<ship_stats_modifier>(s) {}

ssfloat_t::ssfloat_t() : modifiable_ship_stats<sfloat>(){
  for (auto &x : stats) x = 0;
}
ssfloat_t::ssfloat_t(const ssfloat_t &s) : modifiable_ship_stats<sfloat>(s) {}

ship_stats::ship_stats() : ssfloat_t(){
  depends_facility_level = 0;
  build_time = 0;
}

ship_stats::ship_stats(const ship_stats &s) : ssfloat_t(s) {
  ship_class = s.ship_class;
  tags = s.tags;
  upgrades = s.upgrades;
  depends_tech = s.depends_tech;
  depends_facility_level = s.depends_facility_level;
  build_cost = s.build_cost;
  build_time = s.build_time;
  shape = s.shape;
}
