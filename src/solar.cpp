#include "solar.h"

#include <rapidjson/document.h>

#include <iostream>
#include <mutex>
#include <sstream>
#include <vector>

#include "choice.h"
#include "cost.h"
#include "game_data.h"
#include "interaction.h"
#include "research.h"
#include "serialization.h"
#include "utility.h"

using namespace std;
using namespace st3;

const string solar::class_id = "solar";
// const float solar::f_growth = 2e-2;
// const float solar::f_crowding = 2e-3;
// const float solar::f_minerate = 3e-4;
// const float solar::f_buildrate = 7e-4;
// const float solar::f_devrate = 7e-4;
// const float solar::f_resrate = 7e-4;

solar::solar(const solar &s) : game_object(s) {
  *this = s;
};

void solar::pre_phase(game_data *g) {
  research_level = &g->players[owner].research_level;
}

// so far, solars don't move
void solar::move(game_data *g) {
  if (owner < 0 || population() <= 0) return;

  dynamics(g);

  // check for turret combat interaction
  target_condition cond(target_condition::enemy, ship::class_id);
  list<combid> buf = g->search_targets_nophys(id, position, interaction_radius(), cond.owned_by(owner));

  if (buf.size()) {
    // solar combat
    float dlev = effective_level(keywords::key_defense);
    for (int i = 0; i < dlev; i++) {
      combid sid = utility::uniform_sample(buf);
      ship::ptr s = g->get_ship(sid);

      g->log_ship_fire(id, s->id);

      float d = utility::l2norm(s->position - position);
      float ack = dlev * accuracy_distance_norm / (d + 1);
      if (s->evasion_check() < ack) {
        s->receive_damage(g, this, utility::random_uniform(0, dlev));
      }
    }
  }
}

set<string> solar::compile_interactions() {
  return {};
}

float solar::interaction_radius() {
  return radius + 40 + 20 * development[keywords::key_defense];
}

float solar::max_hp() {
  return 30 + 10 * development[keywords::key_defense];
}

void solar::receive_damage(game_object::ptr s, float damage, game_data *g) {
  g->log_bombard(s->id, id);

  hp -= damage;
  // population *= 1 - 0.01 * damage;

  if (owner != game_object::neutral_owner && hp <= 0) {
    owner = s->owner;
    hp = 0.3 * max_hp();

    // switch owners for ships on solar
    for (auto sid : ships) g->get_ship(sid)->owner = owner;
  }
}

void solar::post_phase(game_data *g) {
  if (owner != game_object::neutral_owner && population() == 0) {
    // everyone here is dead, this is now a neutral solar
    owner = game_object::neutral_owner;
  }

  // reg hp
  if (owner != game_object::neutral_owner) {
    hp = fmin(hp + 1, max_hp());
  }
}

void solar::give_commands(list<command> c, game_data *g) {
  hm_t<string, list<combid> > buf;

  // create fleets
  for (auto &x : c) {
    buf.clear();
    for (auto i : x.ships) {
      if (!ships.count(i)) throw logical_error("solar::give_commands: invalid ship id: " + i);
      string sc = g->get_ship(i)->ship_class;
      buf[sc].push_back(i);
    }

    x.origin = id;
    for (auto i : buf) {
      if (!g->allow_add_fleet(owner)) break;

      fleet::ptr f = g->generate_fleet(position, owner, x, i.second);
      if (!f) break;

      for (auto j : f->ships) {
        g->get_ship(j)->on_liftoff(this, g);
        ships.erase(j);
      }
    }
  }
}

bool solar::can_afford(cost::res_t r) {
  for (auto v : keywords::resource)
    if (r[v] > resources[v]) return false;
  return true;
}

void solar::pay_resources(cost::res_t total) {
  for (auto k : keywords::resource) resources[k] = fmax(resources[k] - total[k], 0);
}

string solar::get_info() {
  stringstream ss;
  ss << "pop: " << population() << endl;
  ss << "ships: " << ships.size() << endl;
  return ss.str();
}

sfloat solar::vision() {
  return 1.3 * interaction_radius();
}

solar::ptr solar::create(idtype id, point p, float bounty, float var) {
  float level = pow(2, 13 * bounty);
  auto fres = [level, var]() {
    return fmax(utility::random_normal(level, var * level), 0);
  };

  solar::ptr s = new solar();

  s->id = identifier::make(solar::class_id, id);

  // s -> population = 0;
  s->research_points = 0;
  s->build_progress = -1;
  s->ship_progress = -1;

  for (auto v : keywords::resource) s->resources[v] = fres();
  for (auto v : keywords::development) s->development[v] = 0;

  s->radius = 10 + 7 * sqrt(s->resources.count() / (3 * level));
  s->position = p;
  s->owner = game_object::neutral_owner;
  s->was_discovered = false;

  return s;
}

game_object::ptr solar::clone() {
  return new solar(*this);
}

bool solar::serialize(sf::Packet &p) {
  return p << class_id << *this;
}

float solar::effective_level(string k) {
  return development[k] + research_level->solar_modifier(k);
}

float solar::devtime(string k) {
  return 20 * pow(2, development[k]);
}

cost::res_t solar::devcost(string k) {
  static hm_t<string, cost::res_t> base_cost;
  static bool init = false;

  if (!init) {
    init = true;
    for (auto k : keywords::development) base_cost[k].setup(keywords::resource);

    base_cost[keywords::key_population][keywords::key_metals] = 1;
    base_cost[keywords::key_population][keywords::key_organics] = 2;
    base_cost[keywords::key_population][keywords::key_gases] = 1;

    base_cost[keywords::key_research][keywords::key_metals] = 1;
    base_cost[keywords::key_research][keywords::key_organics] = 1;
    base_cost[keywords::key_research][keywords::key_gases] = 2;

    base_cost[keywords::key_shipyard][keywords::key_metals] = 2;
    base_cost[keywords::key_shipyard][keywords::key_organics] = 1;
    base_cost[keywords::key_shipyard][keywords::key_gases] = 1;

    base_cost[keywords::key_defense][keywords::key_metals] = 1;
    base_cost[keywords::key_defense][keywords::key_organics] = 0;
    base_cost[keywords::key_defense][keywords::key_gases] = 1;
  }

  int level = development[k];
  float multiplier = pow(4, level);
  cost::res_t c = base_cost[k];
  c.scale(multiplier);
  return c;
}

float solar::population() {
  return effective_level(keywords::key_population);
}

void solar::dynamics(game_data *g) {
  float dt = g->get_dt();
  float order = g->solar_order_level(id);

  // research and development
  research_points += order * effective_level(keywords::key_research) * dt;

  if (choice_data.do_produce()) {
    string v = choice_data.ship_queue.front();
    if (research_level->can_build_ship(v, ptr(this))) {
      ship_stats s = ship::table().at(v);

      // check if we are starting to build this ship
      if (ship_progress < 0) {
        if (can_afford(s.build_cost)) {
          ship_progress = 0;
          pay_resources(s.build_cost);
        } else {
          // todo: message can't afford ship
        }
      } else {
        ship_progress += order * sqrt(population()) * effective_level(keywords::key_shipyard) * dt;
      }

      // check ship complete
      if (ship_progress >= s.build_time) {
        ship_progress = -1;
        choice_data.ship_queue.pop_front();

        ship sh = research_level->build_ship(g->next_id(ship::class_id), v);
        sh.states.insert("landed");
        sh.owner = owner;
        ships.insert(sh.id);
        g->add_entity(ship::ptr(new ship(sh)));

        g->players[owner].log.push_back("Completed ship " + v);
      }
    } else {
      // can't build this ship
      g->players[owner].log.push_back("Insufficient resources to build ship " + v);
      choice_data.ship_queue.pop_front();
    }
  }

  if (choice_data.do_develop()) {
    string v = choice_data.building_queue.front();
    if (build_progress < 0) {
      cost::res_t build_cost = devcost(v);
      if (can_afford(build_cost)) {
        build_progress = 0;
        pay_resources(build_cost);
      } else {
        g->players[owner].log.push_back("Insufficient resources to develop " + v);
      }
    } else {
      build_progress += order * population() * dt;
    }

    // check building complete
    if (build_progress >= devtime(v)) {
      build_progress = -1;
      choice_data.building_queue.pop_front();

      development[v]++;
      g->players[owner].log.push_back("Completed building " + v + " level " + to_string(development[v]));
    }
  }
}

bool solar::isa(string c) {
  return c == solar::class_id || c == physical_object::class_id || c == commandable_object::class_id;
}

bool solar::can_see(game_object::ptr x) {
  float r = vision();
  if (!x->is_active()) return false;

  if (x->isa(ship::class_id)) {
    ship::ptr s = utility::guaranteed_cast<ship>(x);
    float area = M_PI * pow(s->radius, 2);
    r = vision() * fmin(area / (s->stats[sskey::key::stealth] + 1), 1);
  }

  float d = utility::l2norm(x->position - position);
  return d < r;
}
