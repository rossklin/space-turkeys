#include "game_data.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <numeric>
#include <queue>
#include <vector>

#include "animation_data.hpp"
#include "com_server.hpp"
#include "fleet.hpp"
#include "game_object.hpp"
#include "interaction.hpp"
#include "research.hpp"
#include "ship.hpp"
#include "solar.hpp"
#include "types.hpp"
#include "upgrades.hpp"
#include "utility.hpp"
#include "waypoint.hpp"

using namespace std;
using namespace st3;
using namespace cost;

int game_data::next_id(class_t x) {
  if (!idc.count(x)) idc[x] = 0;
  return idc[x]++;
}

game_object_ptr game_data::get_game_object(combid i) const {
  if (entity_exists(i)) {
    return get_entity<game_object>(i);
  } else {
    throw logical_error("game_data::get_entity: not found: " + i);
  }
}

ship_ptr game_data::get_ship(combid i) const {
  return get_entity<ship>(i);
}

fleet_ptr game_data::get_fleet(combid i) const {
  return get_entity<fleet>(i);
}

solar_ptr game_data::get_solar(combid i) const {
  return get_entity<solar>(i);
}

waypoint_ptr game_data::get_waypoint(combid i) const {
  return get_entity<waypoint>(i);
}

bool game_data::target_position(combid t, point &p) const {
  if (entity_exists(t)) {
    p = get_game_object(t)->position;
    return true;
  } else {
    return false;
  }
}

// find all entities in ball(p, r) matching condition c
list<combid> game_data::search_targets_nophys(combid self_id, point p, float r, target_condition c, int knn) const {
  list<combid> res;
  game_object_ptr self = get_game_object(self_id);

  for (auto i : entity_grid.search(p, r, knn)) {
    auto e = get_game_object(i.first);
    if (evm.count(self->owner) && evm.at(self->owner).count(i.first) && e->id != self_id && c.valid_on(e)) res.push_back(e->id);
  }

  return res;
}

// find all entities in ball(p, r) matching condition c
list<combid> game_data::search_targets(combid self_id, point p, float r, target_condition c, int knn) const {
  list<combid> res;
  game_object_ptr self = get_game_object(self_id);
  if (!self->isa(physical_object::class_id)) {
    throw logical_error("Non-physical entity called search targets: " + self_id);
  }
  physical_object::ptr s = utility::guaranteed_cast<physical_object>(self);

  for (auto i : entity_grid.search(p, r, knn)) {
    auto e = get_game_object(i.first);
    if (s->can_see(e) && e->id != self_id && c.valid_on(e)) res.push_back(e->id);
  }

  return res;
}

idtype game_data::terrain_at(point p, float r) const {
  for (auto &x : terrain) {
    int j = x.second.triangle(p, r);
    if (j > -1) return x.first;
  }
  return -1;
}

// remove unnecessary path nodes caused by stacked horizons
path_t prune_path(const game_data &g, path_t x, float r) {
  int i, j;
  for (i = 0; i < x.size(); i++) {
    for (j = i + 2; j < x.size(); j++) {
      if (g.first_intersect(x[i], x[j], r) > -1) {
        break;
      }
    }

    for (int k = i + 1; k < j - 1; k++) {
      x.erase(x.begin() + i + 1);
    }
  }

  return x;
}

path_t game_data::get_path(point a, point b, float r) const {
  float eps = 1e-3;
  float r_buffered = (1 - eps) * r;
  int tid;

  // move points outside of terrain
  while ((tid = terrain_at(a, r)) > -1) {
    a = terrain.at(tid).closest_exit(a, r);
  }

  while ((tid = terrain_at(b, r)) > -1) {
    b = terrain.at(tid).closest_exit(b, r);
  }

  tid = first_intersect(a, b, r_buffered);
  if (tid == -1) return path_t(1, b);

  struct ptest {
    float h;
    path_t p;
    pair<int, int> vertice;
  };

  // calculate heuristic cost of path and return as a ptest
  auto gen_ptest = [b](path_t p, pair<int, int> vertice) {
    if (p.empty()) {
      throw logical_error("Empty path in gen_ptest!");
    }

    float h = 0;
    point x = p.front();

    for (auto y : p) {
      h += utility::l2norm(y - x);
      x = y;
    }

    h += utility::l2norm(b - p.back());

    return ptest{h, p, vertice};
  };

  // compare two paths represented by ptest objects by their heuristic cost
  auto ptest_comp = [](ptest a, ptest b) { return a.h > b.h; };

  priority_queue<ptest, vector<ptest>, decltype(ptest_comp)> frontier(ptest_comp);
  frontier.push(gen_ptest(path_t(1, a), {-1, -1}));

  // Adapted A* search
  while (!frontier.empty()) {
    ptest x = frontier.top();
    frontier.pop();
    path_t res = x.p;
    point sub_a = res.back();

    // Check if this is the final solution
    if ((tid = first_intersect(sub_a, b, r_buffered)) == -1) {
      res.push_back(b);
      res.erase(res.begin());
      return prune_path(*this, res, r_buffered);
    }

    // build set of possible state transitions

    // remember blocked vertices
    hm_t<int, set<int>> hr_blocked;

    // block the vertice that corresponds to the end point of the current path
    if (x.vertice.first > -1) hr_blocked[x.vertice.first].insert(x.vertice.second);

    // Find the vertices representing the left and right horizon of a terrain wrt a point
    auto terrain_horizon = [this, r, r_buffered, &hr_blocked](point a, int tid) -> vector<int> {
      terrain_object obj = terrain.at(tid);
      float theta = utility::point_angle(obj.center - a);
      float amax = 0;
      float amin = 0;
      int pmin, pmax;
      bool found_min = false, found_max = false;

      for (int i = 0; i < obj.border.size(); i++) {
        point p = obj.get_vertice(i, r);

        // skip points that are blocked by self
        if (first_intersect(a, p, r_buffered) == tid) continue;

        // skip vertices that are marked as blocked
        if (hr_blocked.count(tid) && hr_blocked.at(tid).count(i)) continue;

        float adiff = utility::angle_difference(utility::point_angle(p - a), theta);
        if (adiff > amax) {
          amax = adiff;
          pmax = i;
          found_max = true;
        }

        if (adiff < amin) {
          amin = adiff;
          pmin = i;
          found_min = true;
        }
      }

      vector<int> res;
      if (found_min) res.push_back(pmin);
      if (found_max) res.push_back(pmax);

      return res;
    };

    vector<int> alts_idx = terrain_horizon(sub_a, tid);

    vector<pair<int, int>> alts;
    for (auto idx : alts_idx) alts.push_back({tid, idx});

    while (alts.size()) {
      pair<int, int> alt = alts.back();
      point q = terrain.at(alt.first).get_vertice(alt.second, r);
      alts.pop_back();

      int tid2;
      if ((tid2 = first_intersect(sub_a, q, r_buffered)) > -1) {
        // block this alt
        hr_blocked[alt.first].insert(alt.second);

        // expand horizon over blocking object
        vector<int> idx2 = terrain_horizon(sub_a, tid2);
        for (auto idx : idx2) alts.push_back({tid2, idx});
      } else {
        path_t new_path = res;
        new_path.push_back(q);
        frontier.push(gen_ptest(new_path, alt));
      }

      if (alts.size() > 1000) {
        server::log("Get path: alt overflow!", "warning");
        return path_t(1, a);
      }
    }

    if (frontier.size() > 1000) {
      server::log("Get path: frontier overflow!", "warning");
      return path_t(1, a);
    }
  }

  // There is not path, e.g. because ship is to big to fit through any passage
  server::log("get_path: no path found");
  return path_t(1, a);
}

int game_data::first_intersect(point a, point b, float r) const {
  // find first intersected terrain object
  hm_t<idtype, list<pair<int, point>>> intersects;
  point inter_buf;
  for (auto &x : terrain) {
    for (int i = 0; i < x.second.border.size(); i++) {
      point p1 = x.second.get_vertice(i, r);
      point p2 = x.second.get_vertice(i + 1, r);
      if (utility::line_intersect(a, b, p1, p2, &inter_buf)) {
        intersects[x.first].push_back(make_pair(i, inter_buf));
      }
    }
  }

  // find starting point
  int tid = -1;
  float closest = INFINITY;

  for (auto &x : intersects) {
    for (auto y : x.second) {
      int bid = y.first;
      point p = y.second;
      float d = utility::l2norm(p - a);
      if (d < closest) {
        closest = d;
        tid = x.first;
      }
    }
  }

  return tid;
}

// create a new fleet and add ships from command c
void game_data::relocate_ships(command c, set<combid> &sh, idtype owner) {
  fleet_ptr f;
  combid source_id;
  set<combid> fleet_buf;

  // check if ships fill exactly one fleet
  for (auto i : sh) {
    ship_ptr s = get_ship(i);
    if (s->has_fleet()) fleet_buf.insert(source_id = s->fleet_id);
  }

  bool reassign = false;
  if (fleet_buf.size() == 1) {
    f = get_fleet(source_id);
    reassign = f->ships == sh;
  }

  if (reassign) {
    c.source = f->com.source;
    c.origin = f->com.origin;
    f->com = c;
  } else {
    f = fleet::create(fleet::server_pid, next_id(fleet::class_id));

    f->com = c;
    f->owner = owner;
    f->update_counter = 0;
    f->com.source = f->id;

    // clear ships from parent fleets
    for (auto i : sh) {
      ship_ptr s = get_ship(i);
      if (s->has_fleet()) {
        fleet_ptr parent = get_fleet(s->fleet_id);
        parent->remove_ship(i);
      }
    }

    // set new fleet id
    for (auto i : sh) {
      ship_ptr s = get_ship(i);
      s->fleet_id = f->id;
    }

    // add the ships
    f->ships = sh;

    // add the fleet
    register_entity(f);
  }

  f->update_data(this, true);
}

// generate a fleet with given ships, set owner and fleet_id of ships
fleet_ptr game_data::generate_fleet(point p, idtype owner, command c, list<combid> sh, bool ignore_limit) {
  if (sh.empty()) return NULL;

  fleet_ptr f = fleet::create(fleet::server_pid, next_id(fleet::class_id));
  f->com = c;
  f->com.source = f->id;
  f->position = p;
  f->radius = settings.fleet_default_radius;
  f->owner = owner;
  float ta = utility::random_uniform(0, 2 * M_PI);
  point tp;
  if (target_position(c.target, tp)) ta = utility::point_angle(tp - p);

  // const int max_ships = get_max_ships_per_fleet(owner);

  for (auto &s : sh) {
    auto sp = get_ship(s);
    f->ships.insert(s);
    sp->owner = owner;
    sp->fleet_id = f->id;
    sp->angle = ta;
    // if (f->ships.size() >= max_ships && !ignore_limit) break;
  }

  register_entity(f);
  distribute_ships(f);
  f->heading = f->position;
  f->update_data(this, true);

  return f;
}

void game_data::apply_choice(choice c, idtype id) {
  // build waypoints and fleets before validating the choice, so that
  // commands based there can be validated
  for (auto &x : c.waypoints) {
    if (identifier::get_multid_owner(x.second->id) != id) {
      throw player_error("apply_choice: player " + to_string(id) + " tried to insert waypoint owned by " + to_string(identifier::get_multid_owner(x.second->id)));
    }
    register_entity(x.second->clone());
  }

  // research
  if (c.research.length() > 0) {
    if (!utility::find_in(c.research, players[id].research_level.available())) {
      throw player_error("Invalid research choice submitted by player " + to_string(id) + ": " + c.research);
    }
  }
  players[id].research_level.researching = c.research;

  // solar choices: require research to be applied
  for (auto &x : c.solar_choices) {
    // validate
    auto e = get_game_object(x.first);

    if (e->owner != id) {
      throw player_error("validate_choice: error: solar choice by player " + to_string(id) + " for " + x.first + " owned by " + to_string(e->owner));
    }

    if (!e->isa(solar::class_id)) {
      throw player_error("validate_choice: error: solar choice by player " + to_string(id) + " for " + x.first + ": not a solar!");
    }

    if (x.second.building_queue.size()) {
      for (auto y : x.second.building_queue) {
        if (!utility::find_in(y, keywords::development)) {
          throw player_error("validate_choice: error: invalid development key: " + y);
        }
      }
    }

    solar_ptr s = get_solar(x.first);

    // reset ship production if altered
    if (s->choice_data.ship_queue.size() && x.second.ship_queue.size() && s->choice_data.ship_queue.front() != x.second.ship_queue.front()) {
      server::log("Reset production of " + s->choice_data.ship_queue.front() + " to " + x.second.ship_queue.front());
      s->ship_progress = -1;
    }

    // apply
    s->choice_data = x.second;
  }

  // commands: validate
  for (auto x : c.commands) {
    auto e = get_game_object(x.first);
    if (e->owner != id) {
      throw player_error("validate_choice: error: command by player " + to_string(id) + " for " + x.first + " owned by " + to_string(e->owner));
    }
  }

  // apply
  for (auto x : c.commands) {
    commandable_object_ptr v = utility::guaranteed_cast<commandable_object>(get_game_object(x.first));
    v->give_commands(x.second, this);
  }
}

void game_data::register_entity(game_object_ptr p) {
  if (entity_exists(p->id)) throw logical_error("add_entity: already exists: " + p->id);
  add_entity(p);
  p->on_add(this);
}

void game_data::deregister_entity(combid i) {
  if (!entity_exists(i)) throw logical_error("remove_entity: " + i + ": doesn't exist!");
  get_game_object(i)->on_remove(this);
  remove_entity(i);
  remove_entities.push_back(i);
}

void game_data::remove_units() {
  auto cl = [this]() {
    auto buf = all_entities<game_object>();
    for (auto e : buf) {
      if (e->remove) deregister_entity(e->id);
    }
  };

  // run twice since fleets may set remove flag when ships are removed
  cl();
  cl();
}

void game_data::distribute_ships(fleet_ptr f) {
  if (f->ships.empty()) return;

  float ship_rad = get_ship(*f->ships.begin())->radius;
  target_condition c(target_condition::any_alignment, ship::class_id);
  c = c.owned_by(f->owner);

  auto pos_occupied = [this, c, ship_rad, f](point p) {
    return terrain_at(p, ship_rad) > -1 || search_targets_nophys(f->id, p, ship_rad, c).size() > 0;
  };

  auto next_position = [this, f, ship_rad]() {
    static float r = 0.1;
    static float a = 0;
    static float a0 = 0;

    point x = f->position + r * utility::normv(a);

    a += 2 * ship_rad / r;
    if (a > a0 + 2 * M_PI - 2 * ship_rad / r) {
      a0 = utility::random_uniform(0, 2 * M_PI);
      a = a0;
      r += 2 * ship_rad;
    }

    return x;
  };

  for (auto sid : f->ships) {
    point test;
    while (pos_occupied(test = next_position())) {
      // dummy loop
    }

    ship_ptr s = get_ship(sid);
    s->position = test;
    entity_grid.insert(s->id, s->position, s->radius);
    evm[s->owner].insert(s->id);
  }

  // float density = 0.02;
  // float area = f->ships.size() / density;
  // float radius = sqrt(area / M_PI);
  // vector<ship_ptr> ships(f->ships.size());

  // transform(f->ships.begin(), f->ships.end(), ships.begin(), [this](combid sid) {
  //   ship_ptr s = get_ship(sid);
  //   s->position = {INFINITY, INFINITY};
  //   return s;
  // });

  // // todo: handle case where player has ships everywhere..?
  // point p = f->position;
  // float test_rad = 1;
  // target_condition c(target_condition::owned, ship::class_id);
  // c = c.owned_by(f->owner);
  // while (search_targets_nophys(f->id, p, radius, c).size() > ceil(f->ships.size() / (float)10)) {
  //   p = f->position + test_rad * utility::normv(utility::random_uniform(0, 2 * M_PI));
  //   test_rad *= 1.2;
  // }

  // auto sample_position = [this, f, c, p, radius](float r) -> point {
  //   point x;
  //   int n = 0;
  //   int count = 0;
  //   do {
  //     x = {utility::random_normal(p.x, radius), utility::random_normal(p.y, radius)};
  //     n = search_targets_nophys(f->id, x, r, c).size();
  //   } while (count++ < 100 && (terrain_at(x, r) > -1 || n > 0));

  //   return x;
  // };

  // for (auto s : ships) {
  //   s->position = sample_position(s->radius);
  //   entity_grid.insert(s->id, s->position);
  //   evm[s->owner].insert(s->id);
  // }
}

void game_data::extend_universe(int i, int j, bool starting_area) {
  if (!settings.enable_extend) return;
  pair<int, int> idx = make_pair(i, j);
  if (discovered_universe.count(idx)) return;
  discovered_universe.insert(idx);

  float ratio = settings.space_index_ratio;
  point ul(i * ratio, j * ratio);
  point br((i + 1) * ratio, (j + 1) * ratio);
  point center = utility::scale_point(ul + br, 0.5);
  float distance = utility::l2norm(center);
  float bounty = exp(-pow(distance / settings.clset.galaxy_radius, 2));
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
  for (auto test : entity_grid.search(center, ratio)) {
    if (identifier::get_type(test.first) == solar::class_id) static_pos.push_back(test.second);
  }

  vector<point> x(n_solar);
  vector<point> v(n_solar, point(0, 0));

  auto point_init = [ul, br]() {
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
        int tid = terrain_at(x[i], 50);
        if (tid > -1 && terrain[tid].triangle(x[i], 50) > -1) {
          x[i] += utility::normalize_and_scale(x[i] - terrain[tid].center, e);
        }
      }
    }
  }

  // make solars
  for (auto p : x) register_entity(solar::create(next_id(solar::class_id), p, bounty, var));

  // add impassable terrain
  static int terrain_idc = 0;
  static mutex m;

  float min_length = 10;
  auto avoid_point = [this, min_length](terrain_object &obj, point p, float rad) -> bool {
    auto j = obj.triangle(p, rad);
    if (j > -1) {
      float test = utility::triangle_relative_distance(obj.center, obj.get_vertice(j, rad), obj.get_vertice(j + 1, rad), p);
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

  vector<solar_ptr> all_solars = filtered_entities<solar>();
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
      for (auto p : all_solars) passed &= utility::l2norm(obj.center - p->position) > p->radius;
      for (auto p : terrain) passed &= utility::l2norm(obj.center - p.second.center) > min_dist + 2 * min_length;
    }

    if (!passed) continue;

    // adapt other terrain to not cover the center
    for (auto &x : terrain)
      if (!avoid_point(x.second, obj.center, min_dist + 2 * min_length)) continue;

    // generate random border
    for (float angle = 0; angle < 2 * M_PI - 0.1; angle += utility::random_uniform(0.2, 0.5)) {
      float rad = fmax(utility::random_normal(120, 20), min_length);
      obj.border.push_back(obj.center + rad * utility::normv(angle));
    }

    // go through solars and make sure we don't cover them
    for (auto p : all_solars)
      if (!avoid_point(obj, p->position, p->radius)) continue;

    // go through terrain so we don't overlap
    bool failed = false;
    float rad = 10;
    for (auto &x : terrain) {
      pair<int, int> test;
      while ((test = obj.intersects_with(x.second, rad)).first > -1) {
        int i = test.first;
        point d1 = obj.get_vertice(i) - obj.center;
        point d2 = obj.get_vertice(i + 1) - obj.center;

        obj.set_vertice(i, obj.center + utility::normalize_and_scale(d1, 0.9 * utility::l2norm(d1)));
        obj.set_vertice(i + 1, obj.center + utility::normalize_and_scale(d2, 0.9 * utility::l2norm(d2)));
        if (fmin(utility::l2norm(d1), utility::l2norm(d2)) < min_length) {
          failed = true;
          break;
        }
      }

      if (!failed) {
        for (auto y : x.second.border)
          if (!avoid_point(obj, y, min_dist)) continue;
        for (auto y : obj.border)
          if (!avoid_point(x.second, y, min_dist)) continue;
        auto test = obj.intersects_with(x.second, rad);
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
  for (auto w : filtered_entities<waypoint>()) {
    int tid = terrain_at(w->position, 0);
    if (tid > -1 && terrain[tid].triangle(w->position, 0) > -1) {
      deregister_entity(w->id);
      break;
    }
  }
}

void game_data::discover(point x, float r, bool starting_area) {
  float ratio = settings.space_index_ratio;
  auto point2idx = [ratio](float u) { return floor(u / ratio); };
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
  for (auto e : filtered_entities<physical_object>())
    if (e->is_active()) discover(e->position, e->vision());
}

void game_data::rebuild_evm() {
  evm.clear();

  // rebuild vision matrix
  for (auto e : all_entities<game_object>()) {
    if (e->is_active() && e->owner != game_object::neutral_owner) {
      // entity sees itself
      evm[e->owner].insert(e->id);

      // only physical entities can see others
      if (e->isa(physical_object::class_id)) {
        physical_object::ptr f = utility::guaranteed_cast<physical_object>(e);
        for (auto i : entity_grid.search(f->position, f->vision())) {
          if (i.first == e->id) continue;

          game_object_ptr t = get_game_object(i.first);
          // only see other players' entities if they are physical and active
          if (t->isa(physical_object::class_id) && t->is_active() && f->can_see(t)) {
            evm[e->owner].insert(t->id);

            // the player will remember seing this solar
            if (t->isa(solar::class_id)) get_solar(t->id)->known_by.insert(e->owner);
          }
        }
      }
    }
  }
}

solar_ptr game_data::closest_solar(point p, idtype id) const {
  solar_ptr s = 0;

  auto buf = filtered_entities<solar>(id);
  if (buf.size()) {
    s = utility::value_min(
        buf,
        (function<float(solar_ptr)>)[ this, p, id ](solar_ptr t)->float {
          return utility::l2d2(t->position - p);
        });
  }
  return s;
}

void game_data::increment() {
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
  for (auto e : all_entities<game_object>()) {
    if (e->is_active()) e->pre_phase(this);
  }

  for (auto e : all_entities<game_object>()) {
    if (e->is_active()) e->move(this);
  }

  // perform interactions
  random_shuffle(interaction_buffer.begin(), interaction_buffer.end());
  auto itab = interaction::table();
  for (auto x : interaction_buffer) {
    interaction i = itab[x.interaction];
    game_object_ptr s = get_game_object(x.source);
    game_object_ptr t = get_game_object(x.target);
    if (i.condition.owned_by(s->owner).valid_on(t)) {
      i.perform(s, t, this);
    }
  }

  for (auto e : all_entities<game_object>()) {
    if (e->is_active()) e->post_phase(this);
  }

  remove_units();
}

void game_data::build_players(vector<server_cl_socket_ptr> clients) {
  // build player data
  vector<sint> colbuf = utility::different_colors(clients.size());
  int i = 0;
  player p;
  for (auto x : clients) {
    p.name = x->name;
    p.color = colbuf[i++];
    players[x->id] = p;
  }
}

// players and settings should be set before build is called
void game_data::build() {
  static research::data rbase;

  if (players.empty()) throw classified_error("game_data: build: no players!", "error");

  auto make_home_solar = [this](point p, idtype pid) {
    cost::res_t initial_resources;
    for (auto v : keywords::resource) initial_resources[v] = 1000;

    solar_ptr s = solar::create(next_id(solar::class_id), p, 1);
    s->owner = pid;
    s->was_discovered = true;
    s->resources = initial_resources;
    // s -> population = 1;
    s->radius = settings.solar_meanrad;
    s->development[keywords::key_population] = 1;
    s->development[keywords::key_shipyard] = 1;
    s->development[keywords::key_research] = 1;
    s->development[keywords::key_defense] = 1;

    for (auto px : players) s->known_by.insert(px.first);

    // debug: start with some ships
    hm_t<string, int> starter_fleet;
    if (settings.clset.starting_fleet == "single") {
      starter_fleet["fighter"] = 1;
    } else if (settings.clset.starting_fleet == "voyagers") {
      starter_fleet["voyager"] = 2;
    } else if (settings.clset.starting_fleet == "battleships") {
      starter_fleet["battleship"] = 40;
    } else if (settings.clset.starting_fleet == "fighters") {
      starter_fleet["fighter"] = 40;
    } else if (settings.clset.starting_fleet == "massive") {
      for (auto sc : ship::all_classes()) starter_fleet[sc] = 500;
    } else {
      throw player_error("Invalid starting fleet option: " + settings.clset.starting_fleet);
    }

    for (auto sc : starter_fleet) {
      for (int j = 0; j < sc.second; j++) {
        ship_ptr sh = rbase.build_ship(next_id(ship::class_id), sc.first);
        if ((!sh->depends_tech.empty()) && settings.clset.starting_fleet == "massive") {
          sh->upgrades += research::data::get_tech_upgrades(sh->ship_class, sh->depends_tech);
        }
        sh->states.insert("landed");
        sh->owner = pid;
        s->ships.insert(sh->id);
        register_entity(sh);
      }
    }
    register_entity(s);
    discover(s->position, 0, true);

    server::log("Created home solar " + s->id + " owned by " + to_string(s->owner));
  };

  float angle = utility::random_uniform(0, 2 * M_PI);
  float np = players.size();
  for (auto &p : players) {
    point p_base = utility::scale_point(utility::normv(angle), settings.clset.galaxy_radius);
    point p_start = utility::random_point_polar(p_base, 0.2 * settings.clset.galaxy_radius);
    make_home_solar(p_start, p.first);
    angle += 2 * M_PI / np;
  }

  // generate universe between players
  int idx_max = floor(settings.clset.galaxy_radius / settings.space_index_ratio);
  for (int i = -idx_max; i <= idx_max; i++) {
    for (int j = -idx_max; j <= idx_max; j++) {
      extend_universe(i, j);
    }
  }
}

// clean up things that will be reloaded from client
void game_data::pre_step() {
  // clear waypoints, but don't list removals as client
  // manages them
  for (auto i : filtered_entities<waypoint>()) i->remove = true;
  remove_units();
  remove_entities.clear();

  // update research facility level for use in validate choice
  update_research_facility_level();
}

// compute max levels for each player and development
void game_data::update_research_facility_level() {
  hm_t<idtype, hm_t<string, int>> level;
  for (auto s : filtered_entities<solar>()) {
    if (s->owner > -1) {
      for (auto v : keywords::development) {
        level[s->owner][v] = max(level[s->owner][v], (int)s->effective_level(v));
      }
    }
  }

  for (auto &x : players) {
    x.second.research_level.facility_level = level[x.first];
  }
}

// Run solar dynamics, pool research and remove unused waypoints
void game_data::end_step() {
  bool check;
  list<combid> remove;

  for (auto i : filtered_entities<waypoint>()) {
    check = false;

    // check for fleets targeting this waypoint
    for (auto j : filtered_entities<fleet>()) {
      check |= j->com.target == i->id;
    }

    // check for waypoints with commands targeting this waypoint
    for (auto j : filtered_entities<waypoint>()) {
      for (auto &k : j->pending_commands) {
        check |= k.target == i->id;
      }
    }

    if (!check) remove.push_back(i->id);
  }

  for (auto i : remove) {
    deregister_entity(i);
  }

  // clear log
  for (auto &x : players) x.second.log.clear();

  // Run solar dynamics
  for (auto s : filtered_entities<solar>()) s->dynamics(this);

  // pool research
  update_research_facility_level();

  hm_t<idtype, float> pool;
  hm_t<idtype, int> count;
  for (auto i : filtered_entities<solar>()) {
    if (i->owner > -1) {
      pool[i->owner] += i->research_points;
      if (i->research_points > 0) count[i->owner]++;
      i->research_points = 0;
    }
  }

  for (auto x : players) {
    idtype id = x.first;
    research::data &r = players[id].research_level;
    bool rcult = r.researched().count("research culture");

    // apply
    if (r.researching.length() > 0) {
      if (utility::find_in(r.researching, r.available())) {
        research::tech &t = r.access(r.researching);

        // inefficiency for multiple research centers if research culture not developed
        float scale = pow(rcult ? 1 : 1.2, -count[id]);
        t.progress += scale * pool[id];

        // check research complete
        if (t.progress >= t.cost_time) {
          t.progress = 0;
          t.level = 1;
          players[id].log.push_back("Completed researching " + r.researching);
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

  auto check_ship_upgrades = [&utab, &stab](hm_t<string, set<string>> u) {
    for (auto &x : u) {
      for (auto v : x.second) assert(utab.count(v));
      if (x.first == research::upgrade_all_ships) continue;
      if (x.first[0] == '!' || x.first[0] == '#' || x.first[0] == '[') continue;
      assert(stab.count(x.first));
    }
  };

  // validate upgrades
  for (auto &u : utab)
    for (auto i : u.second.inter) assert(itab.count(i));

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
}

animation_tracker_info game_data::get_tracker(combid id) const {
  animation_tracker_info info;
  info.eid = id;

  if (entity_exists(id)) {
    info.p = get_game_object(id)->position;

    if (get_game_object(id)->isa(ship::class_id)) {
      ship_ptr s = get_ship(id);
      info.v = s->velocity;
    } else {
      info.v = point(0, 0);
    }
  }

  return info;
}

void game_data::log_bombard(combid a, combid b) {
  ship_ptr s = get_ship(a);
  solar_ptr t = get_solar(b);
  int delay = utility::random_int(settings.clset.sim_sub_frames);

  animation_data x;
  x.t1 = get_tracker(s->id);
  x.t2 = get_tracker(t->id);
  x.color = players[s->owner].color;
  x.cat = animation_data::category::bomb;
  x.magnitude = s->stats[sskey::key::solar_damage];
  x.delay = delay;

  animation_data sh;
  float shield = t->development[keywords::key_defense];
  sh.t1 = get_tracker(t->id);
  sh.radius = 1.2 * t->radius;
  sh.magnitude = shield;
  sh.color = players[t->owner].color;
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
  game_object_ptr s = get_game_object(a);
  ship_ptr t = get_ship(b);
  int delay = utility::random_int(settings.clset.sim_sub_frames);

  animation_data x;
  x.t1 = get_tracker(s->id);
  x.t2 = get_tracker(t->id);
  x.color = players[s->owner].color;
  x.cat = animation_data::category::shot;
  x.delay = delay;

  if (s->isa(ship::class_id)) {
    ship_ptr sp = utility::guaranteed_cast<ship>(s);
    x.magnitude = sp->stats[sskey::key::ship_damage];
  } else {
    x.magnitude = 1;
  }

  animation_data sh;
  sh.t1 = get_tracker(t->id);
  sh.magnitude = t->stats[sskey::key::shield];
  sh.radius = 1.2 * t->radius;
  sh.color = players[t->owner].color;
  sh.cat = animation_data::category::shield;
  sh.delay = delay;

  for (auto &p : players) {
    if (evm[p.first].count(a) || evm[p.first].count(b)) {
      p.second.animations.push_back(x);
    }

    if (evm[p.first].count(b) && t->stats[sskey::key::shield] > 0) {
      p.second.animations.push_back(sh);
    }
  }
}

void game_data::log_ship_destroyed(combid a, combid b) {
  game_object_ptr s = get_game_object(a);
  ship_ptr t = get_ship(b);
  int delay = utility::random_int(settings.clset.sim_sub_frames);

  // text log
  string self_identifier = s->isa(ship::class_id) ? get_ship(a)->ship_class : "solar";
  string log_p1 = "Your " + self_identifier + " destroyed an enemy " + t->ship_class;
  string log_p2 = "Your " + t->ship_class + " was shot down by an enemy " + self_identifier;

  players[s->owner].log.push_back(log_p1);
  players[t->owner].log.push_back(log_p2);

  // animations
  animation_data x;
  x.t1 = get_tracker(t->id);
  x.magnitude = t->stats[sskey::key::mass];
  x.color = players[t->owner].color;
  x.cat = animation_data::category::explosion;
  x.delay = delay;

  for (auto &p : players) {
    if (evm[p.first].count(b)) {
      p.second.animations.push_back(x);
    }
  }
}

void game_data::log_message(combid a, string v_full, string v_short) {
  ship_ptr s = get_ship(a);

  players[s->owner].log.push_back(v_full);

  animation_data x;
  x.t1 = get_tracker(s->id);
  x.cat = animation_data::category::message;
  x.text = v_short;
  x.color = players[s->owner].color;
  x.delay = 0;
  players[s->owner].animations.push_back(x);
}

float game_data::get_dt() const {
  return settings.clset.sim_sub_frames * settings.dt;
}

float game_data::solar_order_level(combid id) const {
  solar_ptr s = get_solar(id);
  idtype pid = s->owner;

  // Get research order modifier
  player p = players.at(pid);
  research::data r = p.research_level;
  float modifier = r.get_order_modifier();

  // Calculate mean and variance of solar positions
  vector<solar_ptr> solars = filtered_entities<solar>(pid);
  float N = solars.size();
  point m = {0, 0};
  float sd = 0;

  for (auto sp : solars) m += sp->position;
  m = 1 / N * m;

  for (auto sp : solars) sd += utility::l2d2(sp->position - m);
  sd = sqrt(1 / N * sd);

  // Calculate relative distance
  float rel_dist = 0;
  if (sd > 0) rel_dist = utility::l2norm(s->position - m) / sd;

  // Calculate total population and number of solars
  float pop = 0;
  for (auto sp : solars) pop += sp->population();
  pop = fmax(pop, 1);

  // Expression for order
  return (1 + modifier) / (pow(N, 0.5) * pow(pop, 0.25) * utility::gaussian_kernel(rel_dist, 1 + modifier));
}

bool game_data::allow_add_fleet(idtype pid) const {
  return true;
  // return get_max_fleets(pid) > filtered_entities<fleet>(pid).size();
}
