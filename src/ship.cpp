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

const string ship::class_id = "ship";
const int ship::na = 10;
const float ship::friction = 1.25;
const float collision_damage_factor = 1e-3;
string ship::starting_ship;

// utility
void apply_ships(game_data *g, list<combid> sids, function<void(ship_ptr)> f) {
  for (combid sid : sids) f(g->get_ship(sid));
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
  fleet_id = identifier::source_none;
  remove = false;
  load = 0;
  nkills = 0;
  skip_head = false;
  radius = 0.5 * pow(stats[sskey::key::mass], 1 / (float)2.1);
  force_refresh = true;
  current_target = "";
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

  // force from neighbours
  apply_ships(g, neighbours, [this, dt](ship_ptr s) {
    // ignore same owner, different fleet
    // if (s->owner == owner && s->fleet_id != fleet_id) return;

    float m1 = stats[sskey::key::mass];
    float m2 = s->stats[sskey::key::mass];
    point v1 = velocity, v2 = s->velocity;
    point x1 = position, x2 = s->position;
    float proj = utility::sproject(v1 - v2, x1 - x2);

    if (proj >= 0) return;  // moving away from each other
    if (utility::l2norm(x2 - x1) >= s->radius + radius) return;

    // transform to zero momentum frame
    point z = 1 / (m1 + m2) * (m1 * v1 + m2 * v2);
    point V1 = v1 - z;
    float beta = 0.2;
    float ad = utility::point_angle(x2 - x1);
    float av = utility::point_angle(V1);
    float theta = utility::angle_difference(ad, av);
    point U1 = beta * utility::l2norm(V1) * utility::normv(av + 2 * theta + M_PI);

    // transform back to original reference frame
    point u1 = U1 + z;

    // apply nessecary force to change velocity in one iteration
    point dvel = u1 - velocity;
    force += m1 / dt * dvel;

    // collision damage
    if (s->owner != owner) {
      collision_damage += collision_damage_factor * s->stats[sskey::key::mass] * utility::l2d2(s->velocity - velocity);
    }
  });

  // accelleration from terrain
  int tid = g->terrain_at(position, radius);
  if (tid > -1) {
    terrain_object x = g->terrain[tid];
    point t = x.closest_exit(position, radius);
    point delta = t - position;
    float d = utility::l2norm(delta);
    float k = 5 * stats[sskey::key::mass] * utility::sigmoid(d, radius) / radius;
    force += utility::normalize_and_scale(delta, k);
  }

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
  force_refresh = false;

  if (!has_fleet()) return;

  fleet_ptr f = g->get_fleet(fleet_id);
  activate = f->stats.converge;

  // Update path variables
  private_path.clear();
  if (utility::l2norm(f->heading - position) < 2 * radius) {
    f->pop_heading = true;
  } else if (g->first_intersect(position, f->heading, radius) > -1) {
    private_path = g->get_path(position, f->heading, radius);
  }

  // Define policy variables
  bool travel = f->com.policy == fleet::policy_maintain_course;
  bool engage = f->com.policy == fleet::policy_aggressive && tags.count("spacecombat");
  bool evade = f->com.policy == fleet::policy_evasive;

  // Calculate angle and speed
  float fleet_target_angle = utility::point_angle(f->heading - f->position);

  if (private_path.size()) {
    target_angle = utility::point_angle(private_path.front() - position);
    target_speed = max_speed();
  } else {
    target_angle = fleet_target_angle;
    target_speed = f->stats.speed_limit;

    if (hpos < -1) {
      // behind fleet
      target_speed = max_speed();
    } else if (hpos > 1) {
      // ahead of fleet
      target_speed = 0.9 * f->stats.speed_limit;
    }
  }

  // Update neighbours, friends and enemies, never require more than 10 neighbours
  float nrad = interaction_radius();
  neighbours = g->search_targets_nophys(
      id,
      position,
      nrad,
      target_condition(target_condition::any_alignment, ship::class_id).owned_by(owner),
      10);

  local_enemies.clear();
  local_friends.clear();

  // precompute enemies and friends
  apply_ships(g, neighbours, [this](ship_ptr s) {
    if (s->owner == owner) {
      local_friends.push_back(s->id);
    } else {
      local_enemies.push_back(s->id);
    }
  });

  // modify speed and angle to engage local enemies
  if (engage) {
    point p;
    float d;
    bool has_target = false;
    if (local_enemies.size() > 0) {
      // head towards a random local enemy
      if (find(local_enemies.begin(), local_enemies.end(), current_target) == local_enemies.end()) {
        current_target = utility::uniform_sample(local_enemies);
      }

      p = g->get_ship(current_target)->position;
      has_target = true;
    } else if (f->stats.enemies.size()) {
      // head towards a fleet identified enemy target
      p = f->stats.enemies.front().first;
      has_target = true;
    }

    if (has_target) {
      d = utility::l2norm(p - position);
      target_angle = utility::point_angle(p - position);
      target_speed = max_speed() * exp(-nrad / (d + 1));
    }
  }

  // flee from fleet enemies
  if (evade && f->stats.can_evade) {
    target_angle = utility::point_angle(f->stats.evade_path - position);
    target_speed = max_speed();
  }

  // let neighbours influence angle and speed
  point tbase = target_speed * utility::normv(target_angle);
  float influence_factor = 0.5;
  if (engage || evade) influence_factor = 0.2;

  if (local_friends.size() > 0) {
    point tn(0, 0);
    apply_ships(g, local_friends, [this, &tn](ship_ptr s) {
      tn += fmin(s->speed(), max_speed()) * utility::normv(s->angle);
    });
    tn = 1 / (float)local_friends.size() * tn;

    tbase = influence_factor * tn + (1 - influence_factor) * tbase;
  }

  target_speed = utility::l2norm(tbase);
  target_angle = utility::point_angle(tbase);
}

// 1. Clean out missing neighbours
// 2. Engage enemies if possible
// 3. Activate fleet action if possible
// 4. Run any upgrade on_move hook interactions
// 5. Update ship angle, velocity and position
void ship::move(game_data *g) {
  auto local_output = [this](string v) { server::output(id + ": move: " + v); };

  // clean up local_*
  list<combid> rbuf;
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
    list<combid> can_hit = utility::filter(local_enemies, [this, g](combid sid) {
      ship_ptr s = g->get_ship(sid);
      return accuracy_check(s) > 0;
    });

    if (can_hit.size()) {
      combid target_id = utility::uniform_sample(can_hit);
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
    if (f->com.target != identifier::target_idle && compile_interactions().count(f->com.action)) {
      game_object_ptr e = g->get_game_object(f->com.target);
      if (can_see(e) && utility::l2norm(e->position - position) <= interaction_radius()) {
        interaction_info info;
        info.source = id;
        info.target = e->id;
        info.interaction = f->com.action;
        g->interaction_buffer.push_back(info);
        local_output("activating " + f->com.action + " on " + e->id);
      } else {
        local_output("can't activate " + f->com.action + " on " + e->id);
      }
    } else {
      local_output("activate: fleet target idle or missing action: " + f->com.action);
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
  position += dt * velocity;

  g->entity_grid.move(id, position);
}

// Receive collision damage
void ship::post_phase(game_data *g) {
  // take collision damage
  if (collision_damage > 0) {
    receive_damage(g, shared_from_this(), collision_damage);
  }
  collision_damage = 0;
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
  return fleet_id != identifier::source_none;
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
