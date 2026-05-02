#include "interaction.hpp"

#include <iostream>
#include <sstream>

#include "fleet.hpp"
#include "game_data.hpp"
#include "ship.hpp"
#include "solar.hpp"
#include "upgrades.hpp"
#include "utility.hpp"

using namespace std;
using namespace st3;
using namespace server;

const class_t target_condition::no_target = "no target";
const string interaction::land = "land";
const string interaction::deploy = "deploy";
const string interaction::search = "search";
const string interaction::auto_search = "auto search";
const string interaction::turret_combat = "turret combat";
const string interaction::space_combat = "space combat";
const string interaction::bombard = "bombard";
const string interaction::colonize = "colonize";
const string interaction::pickup = "pickup";
const string interaction::terraform = "terraform";
const string interaction::hive_support = "hive support";
const string interaction::splash = "splash";

const hm_t<string, interaction> &interaction::table() {
  static bool init = false;
  static hm_t<string, interaction> data;

  if (init) return data;
  interaction i;

  // land
  i.name = interaction::land;
  i.condition = target_condition(target_condition::owned, solar::class_id);
  i.perform = [](game_object_ptr self, game_object_ptr target, game_data *g) {
    ship_ptr s = utility::guaranteed_cast<ship>(self);
    solar_ptr t = utility::guaranteed_cast<solar>(target);

    // unset fleet
    if (s->has_fleet()) {
      g->get_fleet(s->fleet_id)->remove_ship(s->id);
    }
    s->fleet_id = identifier::no_entity;

    // add to solar's fleet
    s->states.insert("landed");
    t->ships.insert(s->id);
  };
  data[i.name] = i;

  // deploy
  i.name = interaction::deploy;
  i.condition = target_condition(target_condition::any_alignment, target_condition::no_target);
  i.perform = [](game_object_ptr self, game_object_ptr null_ptr, game_data *g) {
    ship_ptr s = utility::guaranteed_cast<ship>(self);
    if (s->states.count("deployed")) return;

    s->states.insert("deployed");
    s->stats[sskey::key::interaction_radius] = 100;
    s->stats[sskey::key::vision_range] = 120;
    s->stats[sskey::key::ship_damage] = 10;
    s->stats[sskey::key::solar_damage] = 10;
    s->stats[sskey::key::accuracy] = 5;
    s->stats[sskey::key::thrust] = 0;
  };
  data[i.name] = i;

  // space combat
  i.name = interaction::space_combat;
  i.condition = target_condition(target_condition::enemy, ship::class_id);
  i.perform = [](game_object_ptr self, game_object_ptr target, game_data *g) {
    ship_ptr s = utility::guaranteed_cast<ship>(self);
    ship_ptr t = utility::guaranteed_cast<ship>(target);

    if (s->load < s->stats[sskey::key::load_time]) return;

    g->log_ship_fire(s->id, t->id);

    s->load = 0;
    if (t->evasion_check() < s->accuracy_check(t)) {
      float damage = 0;
      if (s->stats[sskey::key::ship_damage] > 0) {
        damage = utility::random_normal(s->stats[sskey::key::ship_damage], 0.2 * s->stats[sskey::key::ship_damage]);
        damage = fmax(damage, 0);
      }
      t->receive_damage(g, self, damage);

      // check on hit hooks
      for (auto v : s->upgrades) {
        upgrade u = upgrade::table().at(v);
        for (auto i : u.hook["on hit"]) interaction::table().at(i).perform(s, t, g);
      }
    }
  };
  data[i.name] = i;

  // // turret combat
  // i.name = interaction::turret_combat;
  // i.condition = target_condition(target_condition::enemy, ship::class_id);
  // i.perform = [] (game_object_ptr self, game_object_ptr target, game_data *g){
  //   output("interaction: turret_combat: " + self -> id + " targeting " + target -> id);
  //   solar_ptr s = utility::guaranteed_cast<solar>(self);
  //   ship_ptr x = utility::guaranteed_cast<ship>(target);
  //   float d = utility::l2norm(s -> position - x -> position);

  //   for (auto buf : s -> facility_access()){
  //     if (!buf -> is_turret) continue;

  //     turret_t t = s -> developed(buf -> name, 0).turret;

  //     // don't overdo it
  //     if (x -> remove) break;

  //     if (t.damage > 0 && t.load >= 1 && d <= t.range){
  // 	buf -> turret.load = 0;

  // 	g -> log_ship_fire(s -> id, x -> id);

  // 	if (x -> evasion_check() < t.accuracy_check(x, d)){
  // 	  x -> receive_damage(g, s, utility::random_uniform(0, t.damage));
  // 	}
  //     }
  //   }
  // };
  // data[i.name] = i;

  // bombard
  i.name = interaction::bombard;
  i.condition = target_condition(target_condition::enemy, solar::class_id);
  i.perform = [](game_object_ptr self, game_object_ptr target, game_data *g) {
    ship_ptr s = utility::guaranteed_cast<ship>(self);
    solar_ptr t = utility::guaranteed_cast<solar>(target);

    if (s->load < s->stats[sskey::key::load_time]) return;

    // deal damage
    s->load = 0;
    if (utility::random_uniform() < s->stats[sskey::key::accuracy]) {
      t->receive_damage(s, utility::random_uniform(0, s->stats[sskey::key::solar_damage]), g);
    }
  };
  data[i.name] = i;

  // colonize
  i.name = interaction::colonize;
  i.condition = target_condition(target_condition::neutral, solar::class_id);
  i.perform = [](game_object_ptr self, game_object_ptr target, game_data *g) {
    ship_ptr s = utility::guaranteed_cast<ship>(self);
    solar_ptr t = utility::guaranteed_cast<solar>(target);

    t->owner = s->owner;

    s->remove = true;
  };
  data[i.name] = i;

  // // pickup
  // i.name = interaction::pickup;
  // i.condition = target_condition(target_condition::owned, solar::class_id);
  // i.perform = [] (game_object_ptr self, game_object_ptr target, game_data *g){
  //   ship_ptr s = utility::guaranteed_cast<ship>(self);
  //   solar_ptr t = utility::guaranteed_cast<solar>(target);

  //   if (s -> ddata_int("passengers") > 0) return;

  //   int pickup = 1;
  //   int leave = 1;

  //   if (t -> population >= leave + pickup) {
  //     t -> population -= pickup;
  //     s -> dynamic_data["passengers"] = to_string(pickup);
  //   }
  // };
  // data[i.name] = i;

  // hive support
  i.name = hive_support;
  i.perform = [](game_object_ptr self, game_object_ptr null_pointer, game_data *g) {
    ship_ptr s = utility::guaranteed_cast<ship>(self);
    float strength = s->local_friends.size() / 40;
    for (auto sid : s->local_friends) {
      ship_ptr sh = g->get_ship(sid);
      sh->load += strength;
      sh->stats[sskey::key::evasion] = (1 + strength) * sh->base_stats.stats[sskey::key::evasion];
    }
  };
  data[i.name] = i;

  // splash
  i.name = splash;
  i.condition = target_condition(target_condition::enemy, ship::class_id);
  i.perform = [](game_object_ptr self, game_object_ptr target, game_data *g) {
    ship_ptr s = utility::guaranteed_cast<ship>(self);
    ship_ptr t = utility::guaranteed_cast<ship>(target);

    float damage = 0.2 * s->stats[sskey::key::ship_damage];
    target_condition cond(target_condition::any_alignment, ship::class_id);
    auto ns = g->search_targets_nophys(game_object::any_owner, identifier::no_entity, t->position, 20, cond);
    for (auto id : ns) {
      ship_ptr t2 = g->get_ship(id);
      t2->receive_damage(g, s, damage);
    }
  };
  data[i.name] = i;

  init = true;
  return data;
}

bool target_condition::get_alignment(idtype t, idtype s) {
  if (t == game_object::neutral_owner) return target_condition::neutral;
  return s == t ? target_condition::owned : target_condition::enemy;
};

// target condition
target_condition::target_condition() {}

target_condition::target_condition(sint a, class_t w) : alignment(a), what(w), owner(game_object::neutral_owner) {}

target_condition target_condition::owned_by(idtype o) {
  target_condition t = *this;
  t.owner = o;
  return t;
}

bool target_condition::requires_target() {
  return what != no_target;
}

// interaction
bool target_condition::valid_on(game_object_ptr p) {
  bool type_match = !(requires_target() && !p->isa(what));
  sint aligned = 0;
  if (owner == p->owner) {
    aligned = target_condition::owned;
  } else if (p->owner == game_object::neutral_owner) {
    aligned = target_condition::neutral;
  } else {
    aligned = target_condition::enemy;
  }
  bool align_match = aligned & alignment;

  return type_match && align_match;
}
