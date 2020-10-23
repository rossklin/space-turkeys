#include "ship.hpp"

#include <iostream>

#include "fleet.hpp"
#include "game_data.hpp"
#include "interaction.hpp"
#include "serialization.hpp"
#include "solar.hpp"
#include "upgrades.hpp"
#include "utility.hpp"

using namespace std;
using namespace st3;
using namespace utility;

const string ship::class_id = "ship";
const int ship::na = 10;
const float ship::friction = 1.25;
const float collision_damage_factor = 1e-3;
string ship::starting_ship;

// utility
void apply_ships(game_data *g, list<idtype> sids, function<void(ship_ptr)> f) {
  for (idtype sid : sids) f(g->get_ship(sid));
}

ship::ship(const ship &s) : game_object(s), ship_stats(s) {
  *this = s;
};

const hm_t<string, ship_stats> &ship_stats::table() {
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
    a.ship_class = i->name.GetString();

    for (auto j = i->value.MemberBegin(); j != i->value.MemberEnd(); j++) {
      success = false;
      string name = j->name.GetString();
      if (j->value.IsNumber()) {
        float value = j->value.GetDouble();
        success |= a.insert(name, value);

        if (name == "is starting ship") {
          ship::starting_ship = a.ship_class;
          success = true;
        } else if (name == "depends facility level") {
          a.depends_facility_level = value;
          success = true;
        }
      } else if (j->value.IsString()) {
        if (name == "depends tech") {
          a.depends_tech = j->value.GetString();
          success = true;
        }
      } else if (j->value.IsArray()) {
        if (name == "upgrades") {
          for (auto u = j->value.Begin(); u != j->value.End(); u++) a.upgrades.insert(u->GetString());
          success = true;
        } else if (name == "interactions") {
          for (auto u = j->value.Begin(); u != j->value.End(); u++) a.interactions.insert(u->GetString());
          success = true;
        } else if (name == "shape") {
          pair<point, unsigned char> v;
          float rmax = 0;
          for (auto k = j->value.Begin(); k != j->value.End(); k++) {
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
          for (auto k = j->value.Begin(); k != j->value.End(); k++) a.tags.insert(k->GetString());
          success = true;
        }
      } else if (j->value.IsObject()) {
        if (name == "cost") {
          for (auto k = j->value.MemberBegin(); k != j->value.MemberEnd(); k++) {
            string res_name = k->name.GetString();
            float res_value = k->value.GetDouble();
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
  fleet_id = identifier::no_entity;
  remove = false;
  load = 0;
  nkills = 0;
  skip_head = false;
  radius = 0.5 * pow(stats[sskey::key::mass], 1 / (float)2.1);
  force_refresh = true;
  current_target = identifier::no_entity;
  collision_damage = 0;
}

vector<string> ship::all_classes() {
  return utility::hm_keys(table());
}

// 1. Update ship data
// 2. Update hp, shields and loading
// 3. Calculate forces from thrust, neighbours, terrain and friction
void ship::pre_phase(game_data *g) {
  update_data(g);

  // load stuff
  float dt = g->get_dt();
  load = fmin(load + dt, stats[sskey::key::load_time]);
  stats[sskey::key::hp] = fmin(stats[sskey::key::hp] + dt * stats[sskey::key::regeneration], base_stats.stats[sskey::key::hp]);
  stats[sskey::key::shield] = fmin(stats[sskey::key::shield] + dt * 0.01, base_stats.stats[sskey::key::shield]);

  // calculate forces

  // force from drive
  force = thrust * utility::normv(angle);

  // Thrusters towards center
  if (has_fleet() && !fleet_center_blocked) {
    float thr_a = 0.1;
    force += normalize_and_scale(g->get_fleet(fleet_id)->position - position, stats[sskey::key::mass] * thr_a);
  }

  // force from neighbours
  apply_ships(g, neighbours, [this, dt](ship_ptr s) {
    float r = l2norm(s->position - position) - radius - s->radius;

    if (r < 0) {
      const float amax = 1;
      float a = amax * fabs(r) / (radius + s->radius);
      force += normalize_and_scale(position - s->position, stats[sskey::key::mass] * a);
    }

    // ignore same owner, different fleet
    // if (s->owner == owner && s->fleet_id != fleet_id) return;

    // float m1 = stats[sskey::key::mass];
    // float m2 = s->stats[sskey::key::mass];
    // point v1 = velocity, v2 = s->velocity;
    // point x1 = position, x2 = s->position;
    // float proj = utility::sproject(v1 - v2, x1 - x2);

    // if (proj >= 0) return;  // moving away from each other
    // if (utility::l2norm(x2 - x1) >= s->radius + radius) return;

    // // transform to zero momentum frame
    // point z = 1 / (m1 + m2) * (m1 * v1 + m2 * v2);
    // point V1 = v1 - z;
    // float beta = 0.1;
    // float ad = utility::point_angle(x2 - x1);
    // float av = utility::point_angle(V1);
    // float theta = utility::angle_difference(ad, av);
    // point U1 = beta * utility::l2norm(V1) * utility::normv(av + 2 * theta + M_PI);

    // // transform back to original reference frame
    // point u1 = U1 + z;

    // // apply nessecary force to change velocity in one iteration
    // point dvel = u1 - velocity;
    // point dforce = m1 / dt * dvel;

    // // Collisions can not cause more than unit acceleration
    // if (l2norm(dforce) / m1 > 1) dforce *= m1 / l2norm(dforce);

    // force += dforce;

    // // collision damage
    // if (s->owner != owner) {
    //   collision_damage += collision_damage_factor * s->stats[sskey::key::mass] * utility::l2d2(s->velocity - velocity);
    // }
  });

  // // accelleration from terrain
  // int tid = g->terrain_at(position, radius);
  // if (tid > -1) {
  //   terrain_object x = g->terrain[tid];
  //   point t = x.closest_exit(position, radius);
  //   point delta = t - position;
  //   float d = utility::l2norm(delta);
  //   float k = 5 * stats[sskey::key::mass] * utility::sigmoid(d, radius) / radius;
  //   force += utility::normalize_and_scale(delta, k);
  // }

  // force from friction
  force += utility::normalize_and_scale(-velocity, pow(radius, 2) * friction * utility::l2norm(velocity));
}

float ship::speed() {
  return utility::l2norm(velocity);
}

float ship::max_speed() {
  // r^2 * fric * speed = T
  return base_stats.stats[sskey::key::thrust] / (pow(radius, 2) * friction);
}

// Attempt to adjust the target_angle to follow the fleet's path trail
bool ship::follow_fleet_trail(game_data *g) {
  fleet_ptr f = g->get_fleet(fleet_id);
  if (f->path.empty()) return false;

  int i;
  int idx;
  for (i = f->path.size() - 1; i >= 0; i--) {
    if (g->first_intersect(position, f->path[i], buffered_radius()) == -1) break;
  }

  if (i >= 0) {
    idx = i;
  } else {
    idx = -1;
  }
  // }

  if (idx >= 0) {
    target_angle = point_angle(f->path[idx] - position);
    target_speed = max_speed();
    pathing_policy = "trail";
    return true;
  } else {
    return false;
  }
}

// Attempt to follow private path
bool ship::follow_private_path(game_data *g) {
  fleet_ptr f = g->get_fleet(fleet_id);

  if (private_path.size()) {
    if (g->first_intersect(position, private_path.front(), buffered_radius()) > -1) {
      // We are off the path somehow, terrain is in the way
      private_path.clear();
      return false;
    } else {
      target_angle = point_angle(private_path.front() - position);
      target_speed = max_speed();
      pathing_policy = "private";
      return true;
    }
  } else {
    return false;
  }
}

// Attempt to follow fleet heading
bool ship::follow_fleet_heading(game_data *g) {
  if (hpos > 1) return false;

  fleet_ptr f = g->get_fleet(fleet_id);
  float fleet_target_angle = point_angle(f->heading - f->position);
  bool fleet_in_sight = g->first_intersect(position, f->position, buffered_radius()) == -1 && l2norm(f->position - position) < 70;
  bool terrain_ahead = g->first_intersect(position, position + 50 * normv(fleet_target_angle), buffered_radius()) > -1;

  if (fleet_in_sight && !terrain_ahead) {
    // Calculate angle and speed
    target_angle = utility::point_angle(f->heading - f->position);

    if (f->is_idle()) {
      target_speed = 0;
    } else if (hpos < -1) {
      // behind fleet
      target_speed = max_speed();
    } else if (hpos > 1) {
      // ahead of fleet
      target_speed = 0.9 * f->stats.speed_limit;
    } else {
      target_speed = f->stats.speed_limit;
    }
    pathing_policy = "heading";
    return true;
  } else {
    return false;
  }
}

bool ship::build_private_path(game_data *g, point p) {
  private_path = g->get_path(position, p, buffered_radius(3));
  pp_backref = private_path.front();
  private_path.erase(private_path.begin());
  return true;
}

// 1. Unset force refresh
// 2. Clear private path
// 3. If arrived at fleet heading, set f->pop_heading
// 4. If terrain blocks view of fleet heading, calculate private path
// 5. Calculate target angle and target speed
// 6. Update list of local ships, friends and enemies
// 7. Modify angle and speed if engaging or evading
// 8. Modify angle and speed due to neighbours' influence
void ship::update_data(game_data *g) {
  auto local_output = [this](string v) { server::output(id + ": update_data: " + v); };
  if (force_refresh) private_path.clear();
  force_refresh = false;

  if (!has_fleet()) return;

  fleet_ptr f = g->get_fleet(fleet_id);
  activate = f->stats.converge;

  // Define policy variables
  bool travel = f->com.policy == fleet::policy_maintain_course;
  bool engage = f->com.policy == fleet::policy_aggressive && tags.count("spacecombat") && f->stats.enemies.size();
  bool evade = f->com.policy == fleet::policy_evasive && f->stats.can_evade;

  // Update neighbours, friends and enemies, never require more than 10 neighbours
  float nrad = interaction_radius();
  local_friends = g->search_targets_nophys(
      owner,
      id,
      position,
      nrad,
      target_condition(target_condition::owned, ship::class_id).owned_by(owner),
      10);

  local_enemies = g->search_targets_nophys(
      owner,
      id,
      position,
      nrad,
      target_condition(target_condition::enemy, ship::class_id).owned_by(owner),
      10);

  neighbours = append<list<idtype>>(local_friends, local_enemies);

  // clear private path as appropriate
  bool path_opt_found = follow_private_path(g) || follow_fleet_heading(g) || follow_fleet_trail(g);

  // Todo: when enemies spotted, builds private path to fleet center..?

  if (evade) {
    // never clear
    if (private_path.empty() || pathing_policy != "evade") {
      build_private_path(g, f->stats.evade_path);
      path_opt_found = follow_private_path(g);
    }
    pathing_policy = "evade";
  } else if (engage) {
    // clear when we reach some enemies
    if (local_enemies.size() || pathing_policy != "engage") private_path.clear();
    if (private_path.empty() && local_enemies.empty()) {
      build_private_path(g, f->stats.enemies.front().first);
      path_opt_found = follow_private_path(g);
    }
    pathing_policy = "engage";
  } else if (!path_opt_found) {
    float frad = 1.2 * g->get_ship(*f->ships.begin())->radius;
    float f_fill_area = f->ships.size() * M_PI * pow(frad, 2);
    float f_fill_radius = sqrt(f_fill_area / M_PI);

    // If we are already at the fleet, we don't need a private path to get there
    if (f->path.size() > 0 || l2norm(f->position - position) > f_fill_radius) {
      build_private_path(g, f->position);
      path_opt_found = follow_private_path(g);
    }
  }

  if (!path_opt_found) {
    target_speed = 0;
    pathing_policy = "stop";
  }

  // modify speed and angle to engage local enemies
  if (engage && local_enemies.size()) {
    // head towards a random local enemy
    if (find(local_enemies.begin(), local_enemies.end(), current_target) == local_enemies.end()) {
      current_target = utility::uniform_sample(local_enemies);
    }

    point p = g->get_ship(current_target)->position;
    float d = utility::l2norm(p - position);
    target_angle = utility::point_angle(p - position);
    target_speed = max_speed() * exp(-nrad / (d + 1));
  }

  // Alter angle to avoid hitting wall
  auto test_angle = [this, g](float a) { return g->in_terrain(position + 2 * radius * normv(a)); };

  if (test_angle(target_angle)) {
    float fleet_angle = point_angle(f->position - position);
    float delta = angle_difference(fleet_angle, target_angle);
    int dir = signum(delta);

    float upd_angle;
    for (float step = M_PI / 8; step < M_PI / 2 && fabs(step) <= fabs(delta); step += M_PI / 8) {
      upd_angle = target_angle + dir * step;
      if (!test_angle(upd_angle)) {
        target_angle = upd_angle;
        break;
      }
    }
  }

  // let neighbours influence angle and speed
  point tbase = target_speed * utility::normv(target_angle);
  float influence_factor = 0.5;

  if (local_friends.size() > 0 && pathing_policy != "private") {
    point tn(0, 0);
    int nn = 0;
    apply_ships(g, local_friends, [this, &tn, &nn](ship_ptr s) {
      if (scalar_mult(s->position - position, normv(angle)) > 0) {
        tn += fmin(s->speed(), max_speed()) * utility::normv(s->angle);
        nn++;
      }
    });

    if (nn > 0) {
      tn = 1 / (float)nn * tn;
      tbase = influence_factor * tn + (1 - influence_factor) * tbase;
    }
  }

  target_speed = utility::l2norm(tbase);
  target_angle = utility::point_angle(tbase);

  // Check if we use directional thrusters to go towards fleet center
  list<bool> ships_blocking = fmap<list<bool>>(
      local_friends,
      (function<bool(idtype)>)[ this, g, f ](idtype sid) {
        ship_ptr s = g->get_ship(sid);
        bool can_block = dpoint2line(position, f->position, s->position) < s->radius;
        bool close = l2norm(s->position - position) < 1.5 * (radius + s->radius);
        bool blocking_side = scalar_mult(s->position - position, f->position - position) > 0;
        return can_block && close && blocking_side;
      });

  fleet_center_blocked = g->first_intersect(position, f->position, 0) > -1 || any(ships_blocking);
}

// 1. Clean out missing neighbours
// 2. Engage enemies if possible
// 3. Activate fleet action if possible
// 4. Run any upgrade on_move hook interactions
// 5. Update ship angle, velocity and position
void ship::move(game_data *g) {
  auto local_output = [this](string v) { server::output(id + ": move: " + v); };

  // clean up local_*
  list<idtype> rbuf;
  for (auto i : neighbours) {
    if (!(g->entity_exists(i) && can_see(g->get_game_object(i)))) rbuf.push_back(i);
  }

  for (auto i : rbuf) {
    neighbours.remove(i);
    local_friends.remove(i);
    local_enemies.remove(i);
  }

  // shoot enemies if able
  if (compile_interactions().count(interaction::space_combat) && local_enemies.size()) {
    list<idtype> can_hit = utility::filter(local_enemies, [this, g](idtype sid) {
      ship_ptr s = g->get_ship(sid);
      return accuracy_check(s) > 0;
    });

    if (can_hit.size()) {
      idtype target_id = utility::uniform_sample(can_hit);
      // I've got a shot!
      interaction_info info;
      info.source = id;
      info.target = target_id;
      info.interaction = interaction::space_combat;
      g->interaction_buffer.push_back(info);
      local_output("fire at " + target_id);
    } else {
      local_output("can't find a good shot");
    }
  }

  // activate fleet command action if able
  if (activate && has_fleet()) {
    fleet_ptr f = g->get_fleet(fleet_id);
    if (f->com.target != identifier::no_entity && compile_interactions().count(f->com.action)) {
      game_object_ptr e = g->get_game_object(f->com.target);
      if (can_see(e) && utility::l2norm(e->position - position) <= interaction_radius()) {
        interaction_info info;
        info.source = id;
        info.target = e->id;
        info.interaction = f->com.action;
        g->interaction_buffer.push_back(info);
      }
    }
  }

  // check upgrade on_move hooks
  for (auto v : upgrades) {
    upgrade u = upgrade::table().at(v);
    for (auto i : u.hook["on move"]) interaction::table().at(i).perform(shared_from_this(), NULL, g);
  }

  // move
  float dt = g->get_dt();
  float angle_increment = fmin(stats[sskey::key::thrust] / stats[sskey::key::mass], 0.3);
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

  // Check private path
  if (private_path.size() > 0) {
    point x = private_path.front();
    if (scalar_mult(position - x, pp_backref - x) < 0) {
      // Passing from one side of x to the other
      pp_backref = x;
      private_path.erase(private_path.begin());
    }
  }

  position += dt * velocity;

  push_out_of_terrain(g);

  g->entity_grid[owner].move(id, position);
}

// Receive collision damage
void ship::post_phase(game_data *g) {
  // // take collision damage
  // if (collision_damage > 0) {
  //   receive_damage(g, shared_from_this(), collision_damage);
  // }
  // collision_damage = 0;
}

void ship::receive_damage(game_data *g, game_object_ptr from, float damage) {
  float shield_value = stats[sskey::key::shield];

  if (damage > 4 * shield_value) {
    // Shield overpowered
    stats[sskey::key::shield] = 0;
    stats[sskey::key::hp] -= 0.5 * (damage - shield_value);
  } else if (shield_value < 1) {
    // Shield inactive
    stats[sskey::key::hp] -= damage;
  } else {
    // Shield active
    damage = fmax(damage - 1, 0);
    stats[sskey::key::shield] = fmax(shield_value - damage, 0);
  }

  remove = stats[sskey::key::hp] <= 0;

  if (remove) {
    // register kill
    if (from->isa(ship::class_id)) {
      ship_ptr s = utility::guaranteed_cast<ship>(from);
      s->nkills++;
    }

    g->log_ship_destroyed(from->id, id);
  }
}

void ship::on_remove(game_data *g) {
  if (g->entity_exists(fleet_id)) {
    g->get_fleet(fleet_id)->remove_ship(id);
  }
  game_object::on_remove(g);
}

ship_ptr ship::create() {
  return ptr(new ship());
}

game_object_ptr ship::clone() {
  return ptr(new ship(*this));
}

bool ship::serialize(sf::Packet &p) {
  return p << class_id << *this;
}

float ship::vision() {
  return stats[sskey::key::vision_range];
}

set<string> ship::compile_interactions() {
  set<string> res = interactions;
  auto utab = upgrade::table();
  for (auto v : upgrades) res += utab[v].inter;
  return res;
}

// weight accuracy by angular targetability i.e. easier to target
// ships in front of you
float ship::flex_weight(float a) {
  float xw = stats[sskey::key::cannon_flex];
  float da = utility::angle_difference(angle, a);

  return fabs(da) < M_PI * xw;
  // if (da == 0) return 1;

  // // cannon flex transform
  // float xw = stats[sskey::key::cannon_flex];
  // float x = (1 - xw) * utility::safe_inv(xw) * da;
  // return utility::angular_hat(x);
}

float ship::accuracy_check(ship_ptr t) {
  float a = utility::point_angle(t->position - position);
  float d = utility::l2norm(t->position - position);
  return stats[sskey::key::accuracy] * flex_weight(a) * accuracy_distance_norm / (d + 1);
}

float ship::evasion_check() {
  return utility::random_uniform(0, stats[sskey::key::evasion] * evasion_mass_norm / stats[sskey::key::mass]);
}

float ship::interaction_radius() {
  return radius + stats[sskey::key::interaction_radius];
}

bool ship::is_active() {
  return !states.count("landed");
}

bool ship::has_fleet() {
  return fleet_id != identifier::no_entity;
}

void ship::on_liftoff(solar_ptr from, game_data *g) {
  g->players[owner].research_level.repair_ship(shared_from_this());
  states.erase("landed");
  force_refresh = true;
  thrust = 0;
  velocity = {0, 0};
  force = {0, 0};

  for (auto v : upgrades) {
    upgrade u = upgrade::table().at(v);
    game_object_ptr self = shared_from_this();
    game_object_ptr target = utility::guaranteed_cast<game_object, solar>(from);
    for (auto i : u.hook["on liftoff"]) interaction::table().at(i).perform(self, target, g);
  }
}

bool ship::isa(string c) {
  return c == class_id || physical_object::isa(c);
}

bool ship::can_see(game_object_ptr x) {
  float r = vision();
  if (!x->is_active()) return false;

  if (x->isa(ship::class_id)) {
    ship_ptr s = utility::guaranteed_cast<ship>(x);
    float area = pow(s->stats[sskey::key::mass], 2 / (float)3);
    r = vision() * fmin(area * (stats[sskey::key::detection] + 1) / (s->stats[sskey::key::stealth] + 1), 1);
  }

  float d = utility::l2norm(x->position - position);
  return d < r;
}
