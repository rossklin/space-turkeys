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
const int ship::na = 10;
string ship::starting_ship;

const hm_t<string, ship_stats>& ship_stats::table(){
  static bool init = false;
  static hm_t<string, ship_stats> buf;

  if (init) return buf;

  auto pdoc = utility::get_json("ship");
  auto &doc = (*pdoc)["ship"];

  ship_stats s, a;
  bool success;

  // base stats
  s.stats[sskey::key::speed] = 0.5;
  s.stats[sskey::key::hp] = 1;
  s.stats[sskey::key::mass] = 1;
  s.stats[sskey::key::accuracy] = 1;
  s.stats[sskey::key::evasion] = 1;
  s.stats[sskey::key::ship_damage] = 0;
  s.stats[sskey::key::solar_damage] = 0;
  s.stats[sskey::key::interaction_radius] = 20;
  s.stats[sskey::key::vision_range] = 50;
  s.stats[sskey::key::load_time] = 50;
  s.stats[sskey::key::cargo_capacity] = 0;
  s.stats[sskey::key::depends_facility_level] = 0;
  s.stats[sskey::key::build_time] = 100;
  s.stats[sskey::key::regeneration] = 0;
  s.stats[sskey::key::shield] = 0;
  s.stats[sskey::key::detection] = 0;
  s.stats[sskey::key::stealth] = 0;

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
	  float rmax = 0;
	  for (auto k = j -> value.Begin(); k != j -> value.End(); k++) {
	    v.first.x = (*k)["x"].GetDouble();
	    v.first.y = (*k)["y"].GetDouble();
	    v.second = (*k)["c"].GetString()[0];
	    rmax = fmax(rmax, utility::l2norm(v.first));
	    a.shape.push_back(v);
	  }

	  if (rmax == 0) {
	    throw runtime_error("Invalid ship shape for " + a.ship_class);
	  }

	  for (auto &x : a.shape) x.first = utility::scale_point(x.first, 1 / rmax);
	  
	  success = true;
	} else if (name == "tags") {
	  for (auto k = j -> value.Begin(); k != j -> value.End(); k++) a.tags.insert(k -> GetString());
	  success = true;
	}
      }else if (j -> value.IsObject()) {
	if (name == "cost") {
	  for (auto k = j -> value.MemberBegin(); k != j -> value.MemberEnd(); k++) {
	    string res_name = k -> name.GetString();
	    float res_value = k -> value.GetDouble();
	    bool sub_success = cost::parse_resource(res_name, res_value, a.build_cost);

	    if (res_name == "time") {
	      a.build_time = res_value;
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
  radius = pow(stats[sskey::key::mass], 2/(float)3);
  force_refresh = true;
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
  load = fmin(load + 1, base_stats.stats[sskey::key::load_time]);
  stats[sskey::key::hp] = fmin(stats[sskey::key::hp] + stats[sskey::key::regeneration], base_stats.stats[sskey::key::hp]);
  stats[sskey::key::shield] = fmin(stats[sskey::key::shield] + 0.01, base_stats.stats[sskey::key::shield]);
}

bool ship::check_space(float a) {
  if (free_angle.empty()) {
    cout << "check_space: no precomputed angles!" << endl;
    return false;
  }
  return free_angle[utility::angle2index(na, a)] > 10;
};

void ship::update_data(game_data *g) {
  force_refresh = false;
  
  if (!has_fleet()) return;
  fleet::ptr f = g -> get_fleet(fleet_id);
  fleet::suggestion suggest = f -> suggest(id, g);

  auto output = [this] (string v) {
    cout << id << ": update data: " << v << endl;
  };

  bool summon = suggest.id & fleet::suggestion::summon;
  bool engage = suggest.id & fleet::suggestion::engage;
  bool scatter = suggest.id & fleet::suggestion::scatter;
  bool travel = suggest.id & fleet::suggestion::travel;
  activate = suggest.id & fleet::suggestion::activate;
  bool hold = suggest.id & fleet::suggestion::hold;
  bool evade = suggest.id & fleet::suggestion::evade;
  bool local = engage || evade || activate;

  float fleet_target_angle = utility::point_angle(f -> stats.target_position - f -> position);
  point fleet_delta = f -> position - position;
  float fleet_angle = utility::point_angle(fleet_delta);
  target_angle = fleet_target_angle;
  target_speed = f -> stats.speed_limit;

  // analyze neighbourhood
  float nrad = interaction_radius();
  auto local_all_buf = g -> entity_grid -> search(position, nrad);
  for (auto x : local_all_buf) if (x.first != id && g -> get_entity(x.first) -> isa(ship::class_id)) local_all.push_back(x.first);
  neighbours = g -> search_targets(id, position, nrad, target_condition(target_condition::any_alignment, ship::class_id).owned_by(owner));
  local_enemies.clear();
  local_friends.clear();

  auto apply_ships = [this, g] (list<combid> sids, function<void(ship::ptr)> f) {
    for (combid sid : sids) f(g -> get_ship(sid));
  };

  // precompute enemies and friends
  apply_ships(neighbours, [this] (ship::ptr s) {
      if (s -> owner == owner){
	local_friends.push_back(s -> id);
      }else{
	local_enemies.push_back(s -> id);
      }
    });

  // precompute available angles
  free_angle = vector<float>(na, INFINITY);
  apply_ships(neighbours, [this] (ship::ptr s) {
      point delta = s -> position - position;
      float a = utility::point_angle(delta);
      float d = utility::l2norm(delta);
      int i = utility::angle2index(na, a);
      free_angle[i] = fmin(free_angle[i], d);
    });

  // angle and speed for summon
  auto compute_summon = [=] (float &a, float &s) {
    if (travel) {
      float test = abs(utility::angle_difference(fleet_target_angle, fleet_angle));
      output("summon: travel:");
      if (test > 2 * M_PI / 3) {
	// in front of fleet
	if (check_space(fleet_target_angle + M_PI)) {
	  // slow down
	  s = fmin(0.6 * f -> stats.speed_limit, base_stats.stats[sskey::key::speed]);
	  output("summon: in fron of fleet, slowing down");
	}
      } else if (test > M_PI / 3) {
	// side of fleet
	if (check_space(fleet_angle)) {
	  a = fleet_target_angle + 0.15 * utility::angle_difference(fleet_angle, fleet_target_angle);
	  output("summon: to side of fleet, angling in");
	}
      } else {
	// back of fleet
	if (check_space(fleet_target_angle)) {
	  s = base_stats.stats[sskey::key::speed];
	  output("summon: behind fleet, speeding up");
	}
      }
    } else {
      a = fleet_angle;
      output("summon: not traveling - heading towards center of fleet");
    }
  };

  // angle and speed when running local policies
  auto compute_local = [this, suggest, output] (float &a, float &s) {
    point local_delta = suggest.p - position;
    a = utility::point_angle(local_delta);
    s = base_stats.stats[sskey::key::speed];

    output("setting local target angle: " + to_string(a));
    output("setting local target speed: " + to_string(s) + " (delta is " + to_string(utility::l2norm(local_delta)) + ")");
  };

  // compute desired angle and speed
  if (summon) {
    compute_summon(target_angle, target_speed);
    output("summon");
  } else if (local) {
    compute_local(target_angle, target_speed);
    output("local");
  } else if (hold) {
    target_speed = 0;
    output("hold");
  } else if (travel) {
    output("travel");
  } else if (scatter) {
    // just avoid nearby enemies
    if (local_enemies.size()) {
      vector<float> enemies(na, 0);
      apply_ships(local_enemies, [this, &enemies, output](ship::ptr s) {
	  int idx = utility::angle2index(na, utility::point_angle(s -> position - position));
	  enemies[idx] += s -> stats[sskey::key::mass] * s -> stats[sskey::key::ship_damage];
	  output("scatter: avoiding " + s -> id);
	});

      enemies = utility::circular_kernel(enemies, 4);
      target_angle = utility::index2angle(na, utility::vector_min(enemies, utility::identity_function<float>()));
      output("scatter: target angle: " + to_string(target_angle));
    } else {
      target_speed = 0;
      target_angle = angle;
      output("scatter: chilling");
    }
  }

  // allow engage to override local target
  if (engage) {
    if (local_enemies.empty()) {
      // look for enemies a bit further away
      list<combid> t = g -> search_targets(id, position, 2 * nrad, target_condition(target_condition::enemy, ship::class_id).owned_by(owner));
      if (!t.empty()) {
	// target closest
	combid tid = utility::value_min<combid>(t, [this, g] (combid sid) -> float {
	    return utility::l2d2(g -> get_ship(sid) -> position - position);
	  });
	ship::ptr s = g -> get_ship(tid);
	point delta = s -> position - position;
	target_angle = utility::point_angle(delta);
	target_speed = fmax(0, utility::sigmoid(utility::l2norm(delta) - 0.8 * nrad));
      }
    }
  }

  // try to follow ships in front of you
  apply_ships(local_friends, [this](ship::ptr s) {
      float shift = utility::angle_difference(utility::point_angle(s -> position - position), angle);
      float delta = utility::angle_difference(s -> angle, target_angle);
      float w = 0.1;
      target_angle += w * utility::angular_hat(shift) * utility::angular_hat(delta) * delta;
    });   
}

void ship::move(game_data *g){
  if (force_refresh || utility::random_uniform() < 0.1) update_data(g);

  auto output = [this] (string v) {
    cout << id << ": move: " << v << endl;
  };

  // clean up local_*
  list<combid> rbuf;
  for (auto i : neighbours) {
    if (!(g -> entity.count(i) && can_see(g -> get_entity(i)))) rbuf.push_back(i);
  }

  for (auto i : rbuf) {
    neighbours.remove(i);
    local_friends.remove(i);
    local_enemies.remove(i);
  }

  // shoot enemies if able
  if (compile_interactions().count(interaction::space_combat) && local_enemies.size()) {
    auto evalfun = [this, g, output] (combid sid) -> float {
      ship::ptr s = g -> get_ship(sid);
      point delta = s -> position - position;
      float h = utility::safe_inv(cos(utility::angle_difference(utility::point_angle(delta), angle)) + 2);
      output("scanning local enemy: " + s -> id + ": h = " + to_string(h));
      return h;
    };

    combid target_id = utility::value_min<combid>(local_enemies, evalfun);

    if (evalfun(target_id) < 0.4) {
      // I've got a shot!
      interaction_info info;
      info.source = id;
      info.target = target_id;
      info.interaction = interaction::space_combat;
      g -> interaction_buffer.push_back(info);
      output("fire at " + target_id);
    }else{
      output("can't find a good shot");
    }
  }

  // activate fleet command action if able
  if (activate && has_fleet()) {
    fleet::ptr f = g -> get_fleet(fleet_id);
    if (f -> com.target != identifier::target_idle && compile_interactions().count(f -> com.action)) {
      game_object::ptr e = g -> get_entity(f -> com.target);
      if (can_see(e) && utility::l2norm(e -> position - position) <= interaction_radius()) {
	interaction_info info;
	info.source = id;
	info.target = e -> id;
	info.interaction = f -> com.action;
	g -> interaction_buffer.push_back(info);
	output("activating " + f -> com.action + " on " + e -> id);
      } else {
	output("can't activate " + f -> com.action + " on " + e -> id);
      }
    } else {
      output("activate: fleet target idle or missing action: " + f -> com.action);
    }
  }

  // check if there is free space at target angle
  float selected_angle = target_angle;
  float selected_speed = target_speed;
  if (!check_space(selected_angle)) {
    float check_first = 1 - 2 * utility::random_int(2);
    float spread = 0.2 * M_PI;
    
    if (check_space(selected_angle + check_first * spread)) {
      selected_angle = selected_angle + check_first * spread;
      output("blocked: found new space to the right");
    } else {
      check_first *= -1;
      if (check_space(selected_angle + check_first * spread)) {
	selected_angle = selected_angle + check_first * spread;
	output("blocked: found new space to the left");
      } else {
	selected_speed = 0;
	output("blocked: failed to find free space");
      }
    }
  }

  // // check unit collisions
  // for (auto sid : local_all) {
  //   if (g -> entity.count(sid)) {
  //     ship::ptr s = g -> get_ship(sid);
  //     point delta = s -> position - position;
  //     if (utility::l2norm(delta) < radius + s -> radius) {
  // 	g -> collision_buffer.insert(id_pair(id, s -> id));
  // 	output("collided with " + s -> id);
  //     }
  //   }
  // }

  // move
  float angle_increment = fmin(0.2 / sqrt(stats[sskey::key::mass]), 0.5);
  float acceleration = 0.1 * base_stats.stats[sskey::key::speed] / stats[sskey::key::mass];
  float epsilon = 0.01;
  float angle_miss = utility::angle_difference(selected_angle, angle);
  float angle_sign = utility::signum(angle_miss, epsilon);

  if (selected_speed > 0) {
    // check if either side is crowded
    bool crowd_left = !check_space(angle - M_PI / 2);
    bool crowd_right = !check_space(angle + M_PI / 2);
    if (crowd_left && !crowd_right) {
      // make sure target angle is to right of angle
      if (angle_miss < 0){
	selected_angle = angle + 0.1;
	output("avoid collision: edge right");
      }
    } else if (crowd_right && !crowd_left) {
      // make sure target angle is to left of angle
      if (angle_miss > 0) {
	selected_angle = angle - 0.1;
	output("avoid collision: edge left");
      }
    }
  }

  // slow down if missing angle
  float speed_value = fmax(cos(angle_miss), 0) * selected_speed;
  float speed_miss = speed_value - stats[sskey::key::speed];
  float speed_sign = utility::signum(speed_miss, epsilon);
  stats[sskey::key::speed] += fmin(acceleration, fabs(speed_miss)) * speed_sign;
  stats[sskey::key::speed] = fmin(fmax(stats[sskey::key::speed], 0), base_stats.stats[sskey::key::speed]);
  angle += fmin(angle_increment, fabs(angle_miss)) * angle_sign;
  position = position + utility::scale_point(utility::normv(angle), stats[sskey::key::speed]);

  g -> entity_grid -> move(id, position);
}

void ship::post_phase(game_data *g){}

void ship::receive_damage(game_object::ptr from, float damage) {
  stats[sskey::key::shield] = fmax(stats[sskey::key::shield] - 0.1 * damage, 0);
  damage = fmax(damage - stats[sskey::key::shield], 0);
  stats[sskey::key::hp] -= damage;
  remove = stats[sskey::key::hp] <= 0;
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
  return stats[sskey::key::vision_range];
}

set<string> ship::compile_interactions(){
  set<string> res;
  auto utab = upgrade::table();
  for (auto v : upgrades) res += utab[v].inter;
  return res;
}

float ship::accuracy_check(ship::ptr t) {
  float a = utility::point_angle(t -> position - position);
  a = utility::angle_difference(a, angle);
  return utility::random_uniform(0, stats[sskey::key::accuracy] * (cos(a) + 1) / 2);
}

float ship::evasion_check() {
  return utility::random_uniform(0, stats[sskey::key::evasion] / stats[sskey::key::mass]);
}

float ship::interaction_radius() {
  return radius + stats[sskey::key::interaction_radius];
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
  force_refresh = true;
  stats[sskey::key::speed] = 0;

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
  float r = vision();
  if (!x -> is_active()) return false;

  if (x -> isa(ship::class_id)) {
    ship::ptr s = utility::guaranteed_cast<ship>(x);
    float area = pow(s -> stats[sskey::key::mass], 2 / (float)3);
    r = vision() * fmin(area * (stats[sskey::key::detection] + 1) / (s -> stats[sskey::key::stealth] + 1), 1);
  }
  
  float d = utility::l2norm(x -> position - position);
  return d < r;
}

