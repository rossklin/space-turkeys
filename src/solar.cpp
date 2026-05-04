#include "solar.hpp"

#include <rapidjson/document.h>

#include <iostream>
#include <mutex>
#include <sstream>
#include <vector>

#include "choice.hpp"
#include "cost.hpp"
#include "fleet.hpp"
#include "game_data.hpp"
#include "interaction.hpp"
#include "serialization.hpp"
#include "ship.hpp"
#include "upgrades.hpp"
#include "utility.hpp"

using namespace std;
using namespace st3;

const string solar::class_id = "solar";

solar::solar(const solar &s) : game_object(s) {
  *this = s;
};

void solar::pre_phase(game_data *g) {}

// so far, solars don't move
void solar::move(game_data *g) {
  if (owner < 0) return;

  // check for turret combat interaction
  target_condition cond(target_condition::enemy, ship::class_id);
  list<idtype> buf = g->search_targets_nophys(owner, identifier::no_entity, position, interaction_radius(), cond.owned_by(owner));

  if (buf.size()) {
    // solar combat
    float dlev = 1;
    for (int i = 0; i < dlev; i++) {
      idtype sid = utility::uniform_sample(buf);
      ship_ptr s = g->get_ship(sid);

      g->log_ship_fire(id, s->id);

      float d = utility::l2norm(s->position - position);
      float ack = dlev * accuracy_distance_norm / (d + 1);
      if (s->evasion_check() < ack) {
        s->receive_damage(g, shared_from_this(), utility::random_uniform(0, dlev));
      }
    }
  }
}

set<string> solar::compile_interactions() {
  return {};
}

float solar::interaction_radius() {
  return radius + 40;
}

float solar::max_hp() {
  return 30;
}

void solar::receive_damage(game_object_ptr s, float damage, game_data *g) {
  g->log_bombard(s->id, id);

  hp -= damage;

  if (hp <= 0) {
    owner = s->owner;
    hp = 0.3 * max_hp();

    // switch owners for ships on solar
    for (auto sid : ships) g->get_ship(sid)->owner = owner;

    if (owner >= 0 && g->players.count(owner)) {
      std::vector<std::string> available_upgrades;
      for (const auto& u : upgrade::table()) {
        if (!g->players[owner].upgrades.count(u.first)) {
          available_upgrades.push_back(u.first);
        }
      }
      
      if (!available_upgrades.empty()) {
        std::string new_upgrade = utility::uniform_sample(available_upgrades);
        g->players[owner].upgrades[new_upgrade] = upgrade::table().at(new_upgrade);
        g->players[owner].log.push_back("Discovered new technology: " + new_upgrade);
      }
    }
  }
}

void solar::post_phase(game_data *g) {
  // reg hp
  if (owner != game_object::neutral_owner) {
    hp = fmin(hp + 1, max_hp());
  }
}

void solar::give_commands(list<command> c, game_data *g) {
  hm_t<string, list<idtype> > buf;

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

      fleet_ptr f = g->generate_fleet(position, owner, x, i.second);
      if (!f) break;

      for (auto j : f->ships) {
        g->get_ship(j)->on_liftoff(shared_from_this(), g);
        ships.erase(j);
      }
    }
  }
}

sfloat solar::vision() {
  return 1.3 * interaction_radius();
}

solar_ptr solar::create(idtype id, point p, float bounty, float var) {
  float level = pow(2, 13 * bounty);
  auto fres = [level, var]() {
    return fmax(utility::random_normal(level, var * level), 0);
  };

  solar_ptr s(new solar());

  s->id = id;

  s->ship_progress = -1;

  s->radius = 10 + 7 * sqrt(fres() / (3 * level));
  s->position = p;
  s->owner = game_object::neutral_owner;
  s->was_discovered = false;

  return s;
}

game_object_ptr solar::clone() {
  return ptr(new solar(*this));
}

bool solar::serialize(sf::Packet &p) {
  return p << class_id << *this;
}

// Production at end of round
void solar::dynamics(game_data *g) {
  if (owner < 0) return;

  float ship_build_points = 1.0f;

  string v = choice_data.ship_to_build;
  if (v != "") {
    ship_stats s = ship::table().at(v);

    if (ship_progress < 0) {
      ship_progress = 0;
      g->players[owner].log.push_back("Started building " + v);
    }

    bool will_complete = ship_progress + ship_build_points >= s.build_time;

    float needed = s.build_time - ship_progress;
    float use = 0;
    if (needed > 0 && ship_build_points > 0) {
      use = fmin(needed, ship_build_points);
      ship_progress += use;
      ship_build_points -= use;
    }

    if (will_complete) {
      ship_progress = -1;

      ship_ptr sh = ship::build_ship(g->next_id(), v, g->players[owner].upgrades);
      sh->states.insert("landed");
      sh->owner = owner;
      ships.insert(sh->id);
      g->register_entity(sh);

      g->players[owner].log.push_back("Completed ship " + v);
    }
  } else {
    ship_progress = -1;
  }
}

bool solar::isa(string c) {
  return c == class_id || commandable_object::isa(c) || physical_object::isa(c);
}

bool solar::can_see(game_object_ptr x) {
  float r = vision();
  if (!x->is_active()) return false;

  if (x->isa(ship::class_id)) {
    ship_ptr s = utility::guaranteed_cast<ship>(x);
    float area = M_PI * pow(s->radius, 2);
    r = vision() * fmin(area / (s->stats[sskey::key::stealth] + 1), 1);
  }

  float d = utility::l2norm(x->position - position);
  return d < r;
}
