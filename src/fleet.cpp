#include "fleet.hpp"

#include <iostream>
#include <mutex>

#include "game_data.hpp"
#include "interaction.hpp"
#include "serialization.hpp"
#include "ship.hpp"
#include "solar.hpp"
#include "upgrades.hpp"
#include "utility.hpp"
#include "waypoint.hpp"

using namespace std;
using namespace st3;
using namespace utility;

const string fleet::class_id = "fleet";

const string fleet_action::go_to = "go to";
const string fleet_action::idle = "idle";

const sint fleet::policy_aggressive = 1;
// const sint fleet::policy_reasonable = 2;
const sint fleet::policy_evasive = 4;
const sint fleet::policy_maintain_course = 8;

const idtype fleet::server_pid = -1;
const int fleet::update_period = 3; /*!< number of increments between fleet data updates */
const int fleet::interact_d2 = 100; /*!< squared distance from target at which the fleet converges */

// const sint fleet::suggestion::summon = 1;
// const sint fleet::suggestion::engage = 2;
// const sint fleet::suggestion::scatter = 4;
// const sint fleet::suggestion::travel = 8;
// const sint fleet::suggestion::activate = 16;
// const sint fleet::suggestion::hold = 32;
// const sint fleet::suggestion::evade = 64;

fleet::fleet(idtype pid, idtype idx) {
  id = idx;
  radius = 0;
  position = point(0, 0);
  update_counter = 0;
  owner = -1;
  remove = false;
  com.action = fleet_action::idle;
}

fleet::fleet(const fleet &x) : game_object(x) {
  *this = x;
}

sint fleet::default_policy(string action) {
  if (action == interaction::space_combat) {
    return policy_aggressive;
  } else {
    return policy_maintain_course;
  }
}

void fleet::on_remove(game_data *g) {
  for (auto sid : ships) {
    g->get_ship(sid)->fleet_id = identifier::no_entity;
  }
  game_object::on_remove(g);
}

void fleet::pre_phase(game_data *g) {
  check_in_sight(g);
  update_data(g);
  check_action(g);
  // check_waypoint(g);
}

void fleet::move(game_data *g) {
  // linear extrapolation of position estimate
  bool engage = com.policy == fleet::policy_aggressive && stats.enemies.size();
  bool evade = com.policy == fleet::policy_evasive && stats.can_evade;
  if (!(is_idle() || engage || evade)) {
    float d = fmin(stats.speed_limit, l2norm(heading - position));
    position += normalize_and_scale(heading - position, d);
  }
}

void fleet::post_phase(game_data *g) {}

bool fleet::is_active() {
  return !ships.empty();
}

float fleet::vision() {
  return stats.vision_buf;
}

fleet_ptr fleet::create(idtype pid, idtype idx) {
  return fleet_ptr(new fleet(pid, idx));
}

game_object_ptr fleet::clone() {
  return fleet_ptr(new fleet(*this));
}

bool fleet::serialize(sf::Packet &p) {
  return p << class_id << *this;
}

bool st3::fleet::is_idle() {
  return com.action == fleet_action::idle;
}

void fleet::set_idle() {
  com.action = fleet_action::idle;
}

void fleet::give_commands(list<command> c, game_data *g) {
  vector<command> buf(c.begin(), c.end());
  random_shuffle(buf.begin(), buf.end());

  for (auto &x : buf) {
    if (x.ships.empty()) continue;
    fleet_ptr f = create(fleet::server_pid, g->next_id());
    f->com = x;
    f->com.source = f->id;
    if (f->com.origin == identifier::no_entity) f->com.origin = com.origin;
    f->position = position;
    f->radius = radius;
    f->owner = owner;

    point pbuf{0, 0};
    for (auto i : x.ships) {
      if (ships.count(i)) {
        ship_ptr buf = g->get_ship(i);
        buf->fleet_id = f->id;
        remove_ship(i);
        f->ships.insert(i);
        pbuf += buf->position;
      }
    }

    if (f->ships.size() > 0) {
      f->position = 1 / (float)f->ships.size() * pbuf;
      g->register_entity(f);
      f->update_data(g, true);
    } else {
      server::log("fleet::give_commands: no ships matched!", "warning");
    }
  }
}

// cluster enemy ships
void fleet::analyze_enemies(game_data *g) {
  stats.enemies.clear();
  if (ships.empty()) return;

  float r = stats.spread_radius + vision();
  target_condition cond(target_condition::enemy, ship::class_id);
  list<idtype> t = g->search_targets_nophys(owner, id, position, r, cond.owned_by(owner));
  vector<point> pos = fmap<vector<point>>(
      t, (function<point(idtype)>)[g](idtype sid)->point { return g->get_ship(sid)->position; });
  vector<point> x;

  auto local_output = [this](string v) {
    server::output(id + ": analyze enemies: " + v);
  };

  if (t.empty()) {
    stats.can_evade = false;
    local_output("no enemies");
    return;
  }

  x = cluster_points(pos);
  int na = 10;

  // assign ships to clusters
  hm_t<int, float> cc;
  vector<float> scatter_data(na, 1);
  float dps_scale = g->settings.clset.frames_per_round / get_hp();
  for (auto sid : t) {
    ship s(ship::table().at(g->get_ship(sid)->ship_class));
    point p = g->get_ship(sid)->position;

    // scatter value
    int scatter_idx = angle2index(na, point_angle(p - position));
    scatter_data[scatter_idx] += s.get_dps() * dps_scale;

    // cluster assignment
    int idx = vector_min<point>(x, [p](point y) -> float { return l2d2(y - p); });
    cc[idx] += s.get_strength();
  }

  // build enemy fleet stats
  for (auto i : cc) {
    stats.enemies.push_back(make_pair(x[i.first], i.second / get_strength()));
  }

  stats.enemies.sort([](pair<point, float> a, pair<point, float> b) { return a.second > b.second; });

  // find to what degree we are currently facing the enemy
  vector<float> enemy_ckde = circular_kernel(scatter_data, 2);
  float facing = 0;
  for (auto sid : ships) {
    facing += enemy_ckde[angle2index(na, g->get_ship(sid)->angle)];
  }
  facing /= ships.size();
  stats.facing_ratio = facing / (vsum(enemy_ckde) / na);

  // find best evade direction

  // define prioritized directions
  vector<float> dw(na, 1);
  int idx_previous = angle2index(na, point_angle(stats.evade_path - position));
  int idx_target = angle2index(na, point_angle(stats.target_position - position));
  int idx_closest = 0;

  solar_ptr closest_solar = g->closest_solar(position, owner);
  if (closest_solar) {
    idx_closest = angle2index(na, point_angle(closest_solar->position - position));
    dw[idx_closest] = 0.8;
  }

  if (stats.can_evade) dw[idx_previous] = 0.7;

  dw[idx_target] = 0.5;

  // merge direction priorities with enemy strength data via circular kernel
  vector<float> heuristics = elementwise_product(enemy_ckde, circular_kernel(dw, 1));

  int prio_idx;
  try {
    prio_idx = vector_min<float>(heuristics, identity_function<float>());
  } catch (logical_error e) {
    server::log("logical error in vector_min in fleet::analyze_enemies");
    stats.can_evade = false;
    return;
  }

  float evalue = heuristics[prio_idx];

  // only allow evasion if there exists a path with low enemy strength
  if (evalue < 1) {
    stats.can_evade = true;
    stats.evade_path = position + scale_point(normv(index2angle(na, prio_idx)), 100);
  } else {
    stats.can_evade = false;
  }
}

/*! Build the path to the target position */
void fleet::build_path(game_data *g, point t) {
  if (ships.empty()) return;

  // Path needs extra buffering, so buffered radius searches on path nodes do not intersect terrain
  float ship_rad = g->get_ship(*ships.begin())->buffered_radius(3);

  auto buf = g->get_path(position, t, ship_rad);
  path.clear();

  float step_length = 20;
  for (int i = 0; i < buf.size() - 1; i++) {
    point d = buf[i + 1] - buf[i];
    for (float e = 0; e < l2norm(d); e += step_length) {
      point x = buf[i] + normalize_and_scale(d, e);
      path.push_back(x);
    }
  }

  path.push_back(buf.back());
  heading_index = 1;
}

/*! Update the heading index and heading according to path and position */
void fleet::update_heading(game_data *g) {
  if (path.empty() || heading_index < 0) return;

  heading = path[heading_index];
  if (l2norm(position - heading) < 5) {
    // We have (almost) passed the heading in direction of the next point
    heading_index++;

    if (heading_index < path.size()) {
      heading = path[heading_index];
    } else {
      path.clear();
      heading_index = -1;
    }
  }
}

void fleet::update_data(game_data *g, bool set_force_refresh) {
  if (ships.empty()) return;
  stringstream ss;
  int n = ships.size();
  force_refresh |= set_force_refresh;

  // // position, radius, speed and vision
  // point p(0, 0);

  // float rbuf = 0;
  // for (auto k : ships) {
  //   ship_ptr s = g->get_ship(k);
  //   p = p + s->position;
  //   rbuf = max(rbuf, s->radius);
  // }

  radius = g->settings.fleet_default_radius;
  // position = scale_point(p, 1 / (float)n);

  // // need to update fleet data?
  // bool should_update = force_refresh || random_uniform() < 1 / (float)fleet::update_period;

  // always update heading if needed
  if (g->target_position(com.target, stats.target_position)) {
    if (force_refresh) build_path(g, stats.target_position);
    update_heading(g);
  } else {
    path.clear();
    heading_index = -1;
  }

  if (force_refresh) refresh_ships(g);
  force_refresh = false;
  // if (!should_update) return;

  float speed = INFINITY;
  int count;

  // position, radius, speed and vision
  float r2 = 0;
  stats.vision_buf = 0;

  stats.average_ship = ssfloat_t();
  point dir = normalize_and_scale(heading - position, 1);
  float hpossd = 0;
  for (auto k : ships) {
    ship_ptr s = g->get_ship(k);
    speed = fmin(speed, s->max_speed());
    r2 = fmax(r2, l2d2(s->position - position));
    stats.vision_buf = fmax(stats.vision_buf, s->vision());
    for (int i = 0; i < sskey::key::count; i++) stats.average_ship.stats[i] += s->stats[i];

    // heading position stat
    s->hpos = scalar_mult(s->position - position, dir);
    hpossd += pow(s->hpos, 2);
  }

  if (n > 1) {
    hpossd = sqrt(hpossd / n);
    for (auto k : ships) {
      ship_ptr s = g->get_ship(k);
      s->hpos /= hpossd;
    }
  }

  for (int i = 0; i < sskey::key::count; i++) stats.average_ship.stats[i] /= n;
  stats.spread_radius = fmax(sqrt(r2), 10);
  stats.spread_density = n / (M_PI * pow(stats.spread_radius, 2));
  stats.speed_limit = 0.9 * speed;

  analyze_enemies(g);
  check_action(g);
  // suggest_buf = suggest(g);
}

void fleet::refresh_ships(game_data *g) {
  // make ships update data
  for (auto sid : ships) g->get_ship(sid)->force_refresh = true;
}

void fleet::check_action(game_data *g) {
  bool should_refresh = false;

  if (com.target != identifier::no_entity) {
    // check target status valid if action is an interaction
    auto itab = interaction::table();
    game_object_ptr obj = g->get_game_object(com.target);
    if (itab.count(com.action)) {
      target_condition c = itab[com.action].condition.owned_by(owner);
      if (!c.valid_on(obj)) {
        set_idle();
        com.target = identifier::no_entity;
        should_refresh = true;
      }
    }

    // have arrived?
    if (g->target_position(com.target, stats.target_position)) {
      bool buf = false;
      for (auto sid : ships) {
        ship_ptr s = g->get_ship(sid);
        buf |= l2d2(stats.target_position - s->position) < fleet::interact_d2;
      }
      should_refresh |= buf != stats.converge;
      stats.converge = buf;

      if (obj->isa(waypoint::class_id) && l2norm(stats.target_position - position) < 5) {
        // Arrived at waypoint, set idle
        set_idle();
      }
    }
  }

  if (should_refresh) refresh_ships(g);
}

// combid fleet::request_helper_fleet(game_data *g, combid sid) {
//   // Check if there is already an appropriate helper fleet
//   ship_ptr s = g->get_ship(sid);
//   auto fids = helper_fleets;
//   float closest = l2norm(position - s->position);
//   combid best_id = "";
//   for (auto fid : fids) {
//     // Verify that helper fleets still exist
//     if (!g->entity_exists(fid)) {
//       helper_fleets.erase(fid);
//       continue;
//     }

//     // Accept if f is closer and in sight
//     fleet_ptr f = g->get_fleet(fid);
//     if (l2norm(f->position - s->position) < closest && g->first_intersect(s->position, f->position, s->radius) == -1) {
//       closest = l2norm(f->position - s->position);
//       best_id = fid;
//     }
//   }

//   remove_ship(sid);
//   fleet_ptr f;

//   if (best_id.empty()) {
//     // Create a new helper fleet for this ship
//     f = fleet::create(fleet::server_pid, g->next_id());
//     f->com = com;
//     f->com.ships = {sid};
//     f->com.source = f->id;
//     f->position = s->position;
//     f->radius = g->settings.fleet_default_radius;
//     f->owner = owner;
//     f->ships.insert(sid);
//     s->fleet_id = f->id;

//     g->register_entity(f);
//     f->heading = f->position;
//     helper_fleets.insert(f->id);
//   } else {
//     // Use existing fleet
//     f = g->get_fleet(best_id);
//     f->ships.insert(sid);
//     f->com.ships = f->ships;
//   }

//   s->fleet_id = f->id;
//   f->update_data(g, true);

//   // Debug sanity check
//   if (any(fmap<list<bool>>(
//           f->ships, (function<bool(combid)>)[g](combid sid) {
//             ship_ptr s = g->get_ship(sid);
//             return !(l2norm(s->position) > 0 && l2norm(s->position) < 1e4);
//           }))) {
//     throw logical_error("Helper fleet ships at invalid position!");
//   }

//   return f->id;
// }

// void fleet::gather_helper_fleets(game_data *g) {
//   if (helper_fleets.empty()) return;

//   for (auto fid : helper_fleets) {
//     if (!g->entity_exists(fid)) continue;

//     fleet_ptr f = g->get_fleet(fid);
//     f->gather_helper_fleets(g);

//     auto sids = f->ships;
//     for (auto sid : sids) {
//       f->remove_ship(sid);
//       ships.insert(sid);
//       g->get_ship(sid)->fleet_id = id;
//     }
//   }

//   update_data(g, true);
// }

void fleet::check_waypoint(game_data *g) {
  // set to idle and 'land' ships if converged to waypoint
  if (g->entity_exists(com.target) && g->get_game_object(com.target)->isa(waypoint::class_id) && com.action == fleet_action::go_to) {
    if (stats.converge) {
      com.action = fleet_action::idle;
    } else {
      com.action = fleet_action::go_to;
    }
  }
}

void fleet::check_in_sight(game_data *g) {
  if (com.target == identifier::no_entity) return;

  bool seen = g->evm[owner].count(com.target);
  bool solar = g->entity_exists(com.target) && g->get_game_object(com.target)->isa(solar::class_id);
  bool owned = g->entity_exists(com.target) && g->get_game_object(com.target)->owner == owner;

  // target left sight?
  if (!(seen || solar || owned)) {
    set_idle();
  }
}

void fleet::remove_ship(idtype i) {
  ships.erase(i);
  remove = ships.empty();
}

bool fleet::isa(string c) {
  return c == class_id || commandable_object::isa(c);
}

fleet::analytics::analytics() {
  converge = false;
  speed_limit = 0;
  spread_radius = 0;
  spread_density = 0;
  target_position = point(0, 0);
  evade_path = point(0, 0);
  can_evade = false;
  vision_buf = 0;
}

float fleet::get_dps() {
  return stats.average_ship.get_dps() * ships.size();
}

float fleet::get_hp() {
  return stats.average_ship.get_hp() * ships.size();
}

float fleet::get_strength() {
  return stats.average_ship.get_dps() * stats.average_ship.get_hp() * ships.size();
}

// fleet::suggestion::suggestion() {
//   id = 0;
// }

// fleet::suggestion::suggestion(sint i) {
//   id = i;
// }

// fleet::suggestion::suggestion(sint i, point x) {
//   id = i;
//   p = x;
// }
