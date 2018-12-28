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
const float ship::friction = 0.3;
string ship::starting_ship;

// utility
void apply_ships(game_data *g, list<combid> sids, function<void(ship::ptr)> f) {
  for (combid sid : sids) f(g -> get_ship(sid));
}

ship::ship(const ship &s) : game_object(s), ship_stats(s) {
  *this = s;
};

const hm_t<string, ship_stats>& ship_stats::table(){
  static bool init = false;
  static hm_t<string, ship_stats> buf;

  if (init) return buf;

  auto pdoc = utility::get_json("ship");
  auto &doc = (*pdoc)["ship"];

  ship_stats s, a;
  bool success;

  // base stats
  s.stats[sskey::key::thrust] = 0.5;
  s.stats[sskey::key::hp] = 1;
  s.stats[sskey::key::mass] = 1;
  s.stats[sskey::key::accuracy] = 1;
  s.stats[sskey::key::evasion] = 1;
  s.stats[sskey::key::ship_damage] = 0;
  s.stats[sskey::key::solar_damage] = 0;
  s.stats[sskey::key::interaction_radius] = 20;
  s.stats[sskey::key::vision_range] = 50;
  s.stats[sskey::key::load_time] = 10;
  s.stats[sskey::key::cargo_capacity] = 0;
  s.stats[sskey::key::build_time] = 100;
  s.stats[sskey::key::regeneration] = 0;
  s.stats[sskey::key::shield] = 0;
  s.stats[sskey::key::detection] = 0;
  s.stats[sskey::key::stealth] = 0;
  s.stats[sskey::key::cannon_flex] = 0.1;

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
	} else if (name == "interactions") {
	  for (auto u = j -> value.Begin(); u != j -> value.End(); u++) a.interactions.insert(u -> GetString());
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
	    throw parse_error("Invalid ship shape for " + a.ship_class);
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
	      throw parse_error("Invalid cost specification for ship " + a.ship_class + " in: " + res_name);
	    }
	  }
	  success = true;
	}
      }

      if (!success) {
	throw parse_error("Invalid ship term: " + name);
      }
    }
    
    buf[a.ship_class] = a;
  }

  delete pdoc;
  init = true;
  return buf;
}

ship::ship(const ship_stats &s) : ship_stats(s), physical_object() {
  base_stats = s;
  velocity = {0, 0};
  fleet_id = identifier::source_none;
  remove = false;
  load = 0;
  nkills = 0;
  skip_head = false;
  radius = pow(stats[sskey::key::mass], 1/(float)3);
  force_refresh = true;
  current_target = "";
  collision_damage = 0;
}

list<string> ship::all_classes() {
  return utility::get_map_keys(table());
}

int ship::ddata_int(string k) {
  if (dynamic_data.count(k)) {
    return stoi(dynamic_data[k]);
  } else {
    return 0;
  }
}

float ship::ddata_float(string k) {
  if (dynamic_data.count(k)) {
    return stof(dynamic_data[k]);
  } else {
    return 0;
  }
}

string ship::ddata_string(string k) {
  if (dynamic_data.count(k)) {
    return dynamic_data[k];
  } else {
    return "";
  }
}

void ship::pre_phase(game_data *g){
  update_data(g);

  // load stuff
  float dt = g -> get_dt();
  load = fmin(load + dt, stats[sskey::key::load_time]);
  stats[sskey::key::hp] = fmin(stats[sskey::key::hp] + dt * stats[sskey::key::regeneration], base_stats.stats[sskey::key::hp]);
  stats[sskey::key::shield] = fmin(stats[sskey::key::shield] + dt * 0.01, base_stats.stats[sskey::key::shield]);

  // calculate forces

  // force from drive
  force = thrust * utility::normv(angle);

  // force from neighbours
  apply_ships(g, neighbours, [this] (ship::ptr s) {
      // ignore same owner, different fleet
      if (s->owner == owner && s->fleet_id != fleet_id) return;

      point delta = position - s->position;
      float r = utility::l2norm(delta);
      float r2 = utility::l2norm(position + velocity - (s->position + s->velocity)); // look ahead
      float rmean = 0.5 * (r + r2);
      float r0 = radius + s->radius;
      if (rmean >= r0) return;

      float mass = (stats[sskey::key::mass] + s->stats[sskey::key::mass]) / 2;
      float f0 = sqrt(mass);
      float f = f0 * (r0 - rmean) / r0;

      // limit acceleration to 2 ship radius per unit time
      f = fmin(f, stats[sskey::key::mass] * 2 * radius);
      
      point push = utility::normalize_and_scale(delta, f);

      // apply force
      force += push;

      // collision damage
      if (s->owner != owner) {
	collision_damage += s->stats[sskey::key::mass] * utility::l2d2(s->velocity - velocity);
      }
    });
  
  // accelleration from terrain
  int tid = g -> terrain_at(position, 2 * radius);
  if (tid > -1) {
    terrain_object x = g -> terrain[tid];
    point t = x.closest_exit(position, 2 * radius);
    point delta = t - position;
    float d = utility::l2norm(delta);
    float k = 5 * stats[sskey::key::mass] * utility::sigmoid(d, radius) / radius;
    force += utility::normalize_and_scale(delta, k);
  }

  // force from friction
  force += utility::normalize_and_scale(-velocity, pow(radius, 2) * friction * utility::l2norm(velocity));
}

bool ship::check_space(float a) {
  if (free_angle.empty()) {
    server::log("check_space: no precomputed angles!", "warning");
    return false;
  }
  return free_angle[utility::angle2index(na, a)] > 2;
};

float ship::speed() {
  return utility::l2norm(velocity);
}

float ship::max_speed() {
  // r^2 * fric * speed = T
  return base_stats.stats[sskey::key::thrust] / (pow(radius, 2) * friction);
}

void ship::update_data(game_data *g) {
  force_refresh = false;
  activate = false;
  
  if (!has_fleet()) return;
  
  fleet::ptr f = g -> get_fleet(fleet_id);
  activate = f -> stats.converge;

  private_path.clear();
  if (utility::l2norm(f->heading - position) < 2 * radius) {
    f->pop_heading = true;
  } else if (g->first_intersect(position, f->heading, radius) > -1) {
    private_path = g->get_path(position, f->heading, radius);
  }

  auto local_output = [this] (string v) {server::output(id + ": update_data: " + v);};

  bool travel = f -> com.policy == fleet::policy_maintain_course;
  bool engage = f -> com.policy == fleet::policy_aggressive && tags.count("spacecombat");
  bool evade = f -> com.policy == fleet::policy_evasive;

  float fleet_target_angle = utility::point_angle(f -> heading - f -> position);
  target_angle = utility::point_angle(f->heading - position);
  point fleet_delta = f -> position - position;
  float fleet_angle = utility::point_angle(fleet_delta);

  target_speed = f->stats.speed_limit;

  if (hpos < -1) {
    // behind fleet
    target_speed = max_speed();
  } else if (hpos > 1) {
    // ahead of fleet
    target_speed = 0.9 * f->stats.speed_limit;
  }

  if (private_path.size() > 0) {
    target_angle = utility::point_angle(private_path.front() - position);
    target_speed = max_speed();
  }

  // analyze neighbourhood
  float nrad = interaction_radius();
  // auto local_all_buf = g -> entity_grid -> search(position, nrad);
  // local_all.clear();
  // for (auto x : local_all_buf) if (x.first != id && g -> get_entity(x.first) -> isa(ship::class_id)) local_all.push_back(x.first);
  neighbours = g -> search_targets_nophys(id, position, nrad, target_condition(target_condition::any_alignment, ship::class_id).owned_by(owner));
  local_enemies.clear();
  local_friends.clear();

  // precompute enemies and friends
  apply_ships(g, neighbours, [this] (ship::ptr s) {
      if (s -> owner == owner){
	local_friends.push_back(s -> id);
      }else{
	local_enemies.push_back(s -> id);
      }
    });

  // engage local enemies
  if (engage) {
    point p;
    float d;
    if (local_enemies.size() > 0) {
      // head towards a random local enemy
      if (find(local_enemies.begin(), local_enemies.end(), current_target) == local_enemies.end()) {
	current_target = utility::uniform_sample(local_enemies);
      }
    
      p = g -> get_ship(current_target) -> position;
      d = utility::l2norm(p - position);
      target_angle = utility::point_angle(p - position);
      target_speed = max_speed() * exp(-interaction_radius() / (d + 1));
    } else if (f->stats.enemies.size()) {
      p = f->stats.enemies.front().first;
      d = utility::l2norm(p - position);
      target_angle = utility::point_angle(p - position);
      target_speed = max_speed() * exp(-interaction_radius() / (d + 1));
    }
  }

  // flee from fleet enemies
  if (evade && f -> stats.can_evade) {
    target_angle = utility::point_angle(f -> stats.evade_path - position);
    target_speed = max_speed();
  }

  // let neighbours influence angle and speed
  point tbase = target_speed * utility::normv(target_angle);

  if (local_friends.size() > 0) {
    point tn(0, 0);
    apply_ships(g, local_friends, [this, &tn] (ship::ptr s) {
	tn += fmin(s->speed(), max_speed()) * utility::normv(s->angle);
      });
    tn = 1 / (float)local_friends.size() * tn;

    tbase = (float)0.2 * tn + (float)0.8 * tbase;
  } 
    
  target_speed = utility::l2norm(tbase);
  target_angle = utility::point_angle(tbase);

  // // precompute available angles
  // free_angle = vector<float>(na, INFINITY);
  // apply_ships(g, neighbours, [this] (ship::ptr s) {
  //     point delta = s -> position - position;
  //     float a = utility::point_angle(delta);
  //     float d = utility::l2norm(delta) - radius - s -> radius;
  //     int i = utility::angle2index(na, a);
  //     free_angle[i] = fmin(free_angle[i], d);
  //   });

  // float pref_density = 0.2;
  // float pref_maxrad = fmax(sqrt(f -> ships.size() / (M_PI * pref_density)), 20);

  

  // if (engage && !tags.count("spacecombat")) {
  //   engage = false;
  //   scatter = true;
  // }
  // target_angle = fleet_target_angle;
  // target_speed = f -> stats.speed_limit;

  // // pop private path
  // if (private_path.size()) {
  //   point p = private_path.front();
  //   if (utility::l2norm(p - position) < 10) {
  //     private_path.pop_front();
  //   }
    
  //   // if (private_path.size()) {
  //   //   p = private_path.front();

  //   //   // cause ship to be "behind fleet" and speed up
  //   //   fleet_target_angle = fleet_angle = target_angle = utility::point_angle(p - position);
  //   // }
  // }

  // // angle and speed for summon
  // auto compute_summon = [=] (float &a, float &s) {
  //   float test = abs(utility::angle_difference(fleet_target_angle, fleet_angle));
  //   local_output("summon: travel:");
  //   if (test > 2 * M_PI / 3) {
  //     // in front of fleet
  //     // slow down
  //     s = fmin(0.6 * f -> stats.speed_limit, base_stats.stats[sskey::key::speed]);
  //     local_output("summon: in fron of fleet, slowing down");
  //   } else if (test > M_PI / 3) {
  //     // side of fleet
  //     a = fleet_angle;
  //     local_output("summon: to side of fleet, angling in");
  //   } else {
  //     // back of fleet
  //     a = fleet_angle;
  //     if (check_space(a)) {
  // 	s = base_stats.stats[sskey::key::speed];
  // 	local_output("summon: behind fleet, speeding up");
  //     }
  //   }
  // };

  // // angle and speed when running local policies
  // auto compute_local = [this, local_output] (float &a, float &s) {
  //   point local_delta = suggest.p - position;
  //   a = utility::point_angle(local_delta);
  //   s = base_stats.stats[sskey::key::speed];

  //   local_output("setting local target angle: " + to_string(a));
  //   local_output("setting local target speed: " + to_string(s) + " (delta is " + to_string(utility::l2norm(local_delta)) + ")");
  // };

  // // compute desired angle and speed
  // if (summon) {
  //   compute_summon(target_angle, target_speed);
  //   local_output("summon");
  // } else if (local) {
  //   compute_local(target_angle, target_speed);
  //   local_output("local");
  // } else if (hold) {
  //   target_speed = 0;
  //   local_output("hold");
  // } else if (travel) {
  //   local_output("travel");
  // } else if (scatter) {
  //   // just avoid nearby enemies
  //   if (local_enemies.size()) {
  //     vector<float> enemies(na, 0);
  //     apply_ships(g, local_enemies, [this, &enemies, local_output](ship::ptr s) {
  // 	  int idx = utility::angle2index(na, utility::point_angle(s -> position - position));
  // 	  enemies[idx] += s -> stats[sskey::key::mass] * s -> stats[sskey::key::ship_damage];
  // 	  local_output("scatter: avoiding " + s -> id);
  // 	});

  //     enemies = utility::circular_kernel(enemies, 4);
  //     target_angle = utility::index2angle(na, utility::vector_min(enemies, utility::identity_function<float>()));
  //     target_speed = base_stats.stats[sskey::key::speed];
  //     local_output("scatter: target angle: " + to_string(target_angle));
  //   }
  // }

  // // allow engage to override local target
  // if (engage) {
  //   if (local_enemies.empty()) {
  //     // look for enemies a bit further away
  //     list<combid> t = g -> search_targets(id, position, 2 * nrad, target_condition(target_condition::enemy, ship::class_id).owned_by(owner));
  //     if (!t.empty()) {
  // 	// target closest
  // 	combid tid = utility::value_min(t, (function<float(combid)>) [this, g] (combid sid) -> float {
  // 	    return utility::l2d2(g -> get_ship(sid) -> position - position);
  // 	  });
  // 	ship::ptr s = g -> get_ship(tid);
  // 	point delta = s -> position - position;
  // 	target_angle = utility::point_angle(delta);
  // 	target_speed = fmax(0, utility::sigmoid(utility::l2norm(delta) - 0.8 * nrad)) / g -> get_dt();
  //     }
  //   }
  // }

  // if (activate || (engage && local_enemies.size() > 0)) {
  //   // keep it slow when engaging
  //   target_speed = fmin(target_speed, max_local_speed);
  // }

  // // try to follow ships in front of you
  // apply_ships(g, local_friends, [this](ship::ptr s) {
  //     float shift = utility::angle_difference(utility::point_angle(s -> position - position), angle);
  //     float delta = utility::angle_difference(s -> angle, target_angle);
  //     float w = 0.1;
  //     target_angle += w * utility::angular_hat(shift) * utility::angular_hat(delta) * delta;
  //   });   
}

void ship::move(game_data *g) {
  auto local_output = [this] (string v) {server::output(id + ": move: " + v);};

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
    function<float(combid)> evalfun = [this, g, local_output] (combid sid) -> float {
      // todo: don't cheat, use ship class template instead of actual ship!
      ship::ptr s = g -> get_ship(sid);
      float a = accuracy_check(s);
      float b = s -> stats[sskey::key::evasion] / s -> stats[sskey::key::mass];
      if (b < 0.01) b = 0.01;
      float h = a / b;
      return -h;
    };

    combid target_id = utility::value_min(local_enemies, evalfun);

    if (evalfun(target_id) < -0.3) {
      // I've got a shot!
      interaction_info info;
      info.source = id;
      info.target = target_id;
      info.interaction = interaction::space_combat;
      g -> interaction_buffer.push_back(info);
      local_output("fire at " + target_id);
    }else{
      local_output("can't find a good shot");
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
	local_output("activating " + f -> com.action + " on " + e -> id);
      } else {
	local_output("can't activate " + f -> com.action + " on " + e -> id);
      }
    } else {
      local_output("activate: fleet target idle or missing action: " + f -> com.action);
    }
  }

  // check upgrade on_move hooks
  for (auto v : upgrades) {
    upgrade u = upgrade::table().at(v);
    for (auto i : u.hook["on move"]) interaction::table().at(i).perform(this, NULL, g);
  }

  // // check if there is free space at target angle
  // float selected_angle = target_angle;
  // float selected_speed = target_speed;
  // if (!check_space(selected_angle)) {
  //   float check_first = 1 - 2 * utility::random_int(2);
  //   float spread = 0.2 * M_PI;
    
  //   if (check_space(selected_angle + check_first * spread)) {
  //     selected_angle = selected_angle + check_first * spread;
  //     local_output("blocked: found new space to the right");
  //   } else {
  //     check_first *= -1;
  //     if (check_space(selected_angle + check_first * spread)) {
  // 	selected_angle = selected_angle + check_first * spread;
  // 	local_output("blocked: found new space to the left");
  //     } else {
  // 	selected_speed = 0;
  // 	local_output("blocked: failed to find free space");
  //     }
  //   }
  // }

  // if (target_speed > 0) {
  //   // check if either side is crowded
  //   bool crowd_left = !check_space(angle - M_PI / 2);
  //   bool crowd_right = !check_space(angle + M_PI / 2);
  //   if (crowd_left && !crowd_right) {
  //     // make sure target angle is to right of angle
  //     if (angle_miss < 0){
  // 	target_angle = angle + 0.1;
  // 	local_output("avoid collision: edge right");
  //     }
  //   } else if (crowd_right && !crowd_left) {
  //     // make sure target angle is to left of angle
  //     if (angle_miss > 0) {
  // 	target_angle = angle - 0.1;
  // 	local_output("avoid collision: edge left");
  //     }
  //   }
  // }

  // move
  float dt = g -> get_dt();
  float angle_increment = fmin(stats[sskey::key::thrust] / stats[sskey::key::mass], 1);
  float epsilon = dt * 0.01;
  float angle_miss = utility::angle_difference(target_angle, angle);
  float angle_sign = utility::signum(angle_miss, epsilon);

  // slow down if missing angle or going too fast
  float forward_speed = utility::scalar_mult(velocity, utility::normv(target_angle));

  // update thrust, angle, velocity and position
  // r^2 * fric * speed = T
  float required_thrust = pow(radius, 2) * friction * target_speed;
  thrust = fmax(cos(angle_miss), 0) * fmin(required_thrust, stats[sskey::key::thrust]);
  angle += fmin(dt * angle_increment, fabs(angle_miss)) * angle_sign;
  velocity += dt / stats[sskey::key::mass] * force;
  position += dt * velocity;
  
  g -> entity_grid -> move(id, position);

  // float speed_miss = speed_value - stats[sskey::key::speed];
  // float speed_sign = utility::signum(speed_miss, epsilon);
  // stats[sskey::key::speed] += fmin(acceleration, fabs(speed_miss)) * speed_sign;
  // stats[sskey::key::speed] = fmin(fmax(stats[sskey::key::speed], 0), base_stats.stats[sskey::key::speed]);
  // float current_speed = dt * stats[sskey::key::speed];
  // point new_position = position + current_speed * utility::normv(angle);

  // // push out of impassable terrain
  // float t_rad = radius;
  // list<idtype> tids = g -> terrain_at(new_position, t_rad);
  // if (tids.size()) {
  //   int tid = tids.front();
  //   terrain_object x = g -> terrain[tid];
  //   float d_best = INFINITY;
  //   int bid = -1;
  //   point inter, a, b;
  //   for (int i = 0; i < x.border.size() - 1; i++) {
  //     a = x.get_vertice(i, t_rad);
  //     b = x.get_vertice(i + 1, t_rad);
  //     point r;
  //     if (position != new_position && utility::line_intersect(position, new_position, a, b, &r)) {
  // 	float d = utility::l2d2(position - r);
  // 	if (d < d_best) {
  // 	  d_best = d;
  // 	  bid = i;
  // 	  inter = r;
  // 	}
  //     }
  //   }

  //   if (bid == -1) {
  //     if (x.triangle(position, t_rad)) {
  //     	server::log("ship::move: terrain correction: ship already inside terrain!", "warning");
  //     } else {
  //     	server::log("ship::move: terrain correction: no intersection on travel line!", "warning");
  //     }
      
  //     // ship already in terrain, chose closest border
  //     d_best = INFINITY;
  //     bid = -1;
  //     inter, a, b;
  //     for (int i = 0; i < x.border.size() - 1; i++) {
  // 	a = x.get_vertice(i, t_rad);
  // 	b = x.get_vertice(i + 1, t_rad);
  // 	point r;
  // 	float d = utility::dpoint2line(new_position, a, b, &r);
  // 	if (d < d_best) {
  // 	  d_best = d;
  // 	  bid = i;
  // 	  inter = r;
  // 	}
  //     }
  //   }

  //   point d = b - a;
  //   point n = d.y == 0 ? point(0, 1) : point(1, d.x / d.y);
  //   if (utility::l2d2(inter + n - x.center) < utility::l2d2(inter - n - x.center)) {
  //     // chose the normal pointing outwards
  //     n = -n;
  //   }

  //   point corrected_position = inter + utility::normalize_and_scale(n, 0.1) + utility::sproject(new_position - position, d) * d;

  //   if (utility::l2norm(corrected_position - position) > current_speed) {
  //     corrected_position = position + utility::normalize_and_scale(corrected_position - position, current_speed);
  //   }

  //   float an = utility::point_angle(-n);
  //   float test = utility::angle_difference(selected_angle, an);
  //   if (fabs(test) < M_PI / 4) {
  //     require_private_path = true;
  //   }
    
  //   if (utility::l2d2(new_position - position) > 40) {
  //     server::log("ship::move: jump!", "warning");
  //   }

  //   new_position = corrected_position;
  // }

  // position = new_position;
  
  // g -> entity_grid -> move(id, position);
}

void ship::post_phase(game_data *g) {
  // take collision damage
  if (collision_damage > 0) {
    receive_damage(g, this, collision_damage);
  }
  collision_damage = 0;
}

void ship::receive_damage(game_data *g, game_object::ptr from, float damage) {
  float shield_value = stats[sskey::key::shield];
  stats[sskey::key::shield] = fmax(stats[sskey::key::shield] - 0.1 * damage, 0);
  damage = fmax(damage - shield_value, 0);
  stats[sskey::key::hp] -= damage;
  remove = stats[sskey::key::hp] <= 0;

  if (remove) {
    // register kill
    if (from -> isa(ship::class_id)) {
      ship::ptr s = utility::guaranteed_cast<ship>(from);
      s -> nkills++;
    }
    
    g -> log_ship_destroyed(from->id, id);
  }
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

game_object::ptr ship::clone(){
  return new ship(*this);
}

bool ship::serialize(sf::Packet &p){
  return p << class_id << *this;
}

float ship::vision(){
  return stats[sskey::key::vision_range];
}

set<string> ship::compile_interactions(){
  set<string> res = interactions;
  auto utab = upgrade::table();
  for (auto v : upgrades) res += utab[v].inter;
  return res;
}

// weight accuracy by angular targetability i.e. easier to target
// ships in front of you
float ship::flex_weight(float a) {
  float da = utility::angle_difference(angle, a);
  if (da == 0) return 1;

  // cannon flex transform
  float xw = stats[sskey::key::cannon_flex];
  float x = (1 - xw) * utility::safe_inv(xw) * da;
  return utility::angular_hat(x);
}

float ship::accuracy_check(ship::ptr t) {
  float a = utility::point_angle(t -> position - position);
  float d = utility::l2norm(t -> position - position);
  return stats[sskey::key::accuracy] * flex_weight(a) * accuracy_distance_norm / (d + 1);
}

float ship::evasion_check() {
  return utility::random_uniform(0, stats[sskey::key::evasion] * evasion_mass_norm / stats[sskey::key::mass]);
}

float ship::interaction_radius() {
  return radius + stats[sskey::key::interaction_radius];
}

bool ship::is_active(){
  return !states.count("landed");
}

bool ship::has_fleet() {
  return fleet_id != identifier::source_none;
}

void ship::on_liftoff(solar::ptr from, game_data *g){
  g -> players[owner].research_level.repair_ship(*this);
  states.erase("landed");
  force_refresh = true;
  thrust = 0;
  velocity = {0,0};
  force = {0,0};

  for (auto v : upgrades) {
    upgrade u = upgrade::table().at(v);
    for (auto i : u.hook["on liftoff"]) interaction::table().at(i).perform(this, from, g);
  }
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

