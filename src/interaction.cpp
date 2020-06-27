#include "interaction.h"

#include <iostream>
#include <sstream>

#include "fleet.h"
#include "game_data.h"
#include "ship.h"
#include "solar.h"
#include "upgrades.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace server;

const class_t target_condition::no_target = "no target";
const string interaction::trade_to = "trade to";
const string interaction::trade_from = "trade from";
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

void load_resources(ship::ptr s, solar::ptr t) {
  float total = t->resources.count();
  if (total == 0) return;

  float ratio = fmax(s->stats[sskey::key::cargo_capacity] / total, 1);
  cost::res_t move = t->resources;
  move.scale(ratio);
  s->cargo.add(move);
  move.scale(-1);
  t->resources.add(move);
  s->states.insert("loaded");
}

const hm_t<string, interaction> &interaction::table() {
  static bool init = false;
  static hm_t<string, interaction> data;

  if (init) return data;
  interaction i;

  // land
  i.name = interaction::land;
  i.condition = target_condition(target_condition::owned, solar::class_id);
  i.perform = [](game_object::ptr self, game_object::ptr target, game_data *g) {
    output("interaction: land: " + self->id + " targeting " + target->id);
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    solar::ptr t = utility::guaranteed_cast<solar>(target);
    output(s->id + " lands at " + t->id);

    // unset fleet
    if (s->has_fleet()) {
      g->get_fleet(s->fleet_id)->remove_ship(s->id);
    }
    s->fleet_id = identifier::source_none;

    // unload cargo
    t->resources.add(s->cargo);
    s->cargo = cost::res_t();
    s->states.erase("loaded");

    // add to solar's fleet
    s->states.insert("landed");
    t->ships.insert(s->id);
  };
  data[i.name] = i;

  // deploy
  i.name = interaction::deploy;
  i.condition = target_condition(target_condition::any_alignment, target_condition::no_target);
  i.perform = [](game_object::ptr self, game_object::ptr null_ptr, game_data *g) {
    output("interaction: deploy: " + self->id);
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    if (s->states.count("deployed")) return;

    s->states.insert("deployed");
    s->stats[sskey::key::interaction_radius] = 100;
    s->stats[sskey::key::vision_range] = 120;
    s->stats[sskey::key::ship_damage] = 10;
    s->stats[sskey::key::solar_damage] = 10;
    s->stats[sskey::key::thrust] = 0;
  };
  data[i.name] = i;

  // search
  auto do_search = [](game_object::ptr self, game_object::ptr target, game_data *g) {
    output("interaction: search: " + self->id + " targeting " + target->id);
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    solar::ptr t = utility::guaranteed_cast<solar>(target);
    stringstream ss;
    stringstream sshort;

    ss << "Your " << s->ship_class << " searched " << t->id;
    sshort << "Search: ";

    float test = utility::random_uniform();
    bool message_set = false;
    if (t->was_discovered) {
      // nothing happens
    } else if (test < 0.1) {
      // random tech
      auto rtab = research::data::table();
      auto vkeys = utility::hm_keys(rtab);
      set<string> keys(vkeys.begin(), vkeys.end());
      research::data &level = g->players[s->owner].research_level;
      auto researched = level.researched();
      keys = keys - researched;
      set<string> frontier;

      for (auto k : keys) {
        research::tech t = rtab[k];
        bool avail = true;
        for (auto d : t.depends_techs) {
          if (!researched.count(d)) {
            avail = false;
            break;
          }
        }

        if (avail) frontier.insert(k);
      }

      if (keys.size() > 0) {
        float sub_test = utility::random_uniform();
        string tech;

        if (sub_test < 0.1) {
          // low likelihood: sample from all techs
          tech = utility::uniform_sample(keys);
        } else {
          // high likelihood: sample from next in line techs
          tech = utility::uniform_sample(frontier);
        }

        level.access(tech).level = 1;
        ss << " and discovered an ancient technology: " << tech << "!";
        sshort << "discovered tech: " << tech << "!";
        message_set = true;
      }
    } else if (test < 0.2) {
      // population boost
      solar::ptr csol = g->closest_solar(t->position, s->owner);
      if (csol) {
        csol->development[keywords::key_population]++;
        ss << " and encountered a group of people who join your civilization at " << t->id << "!";
        sshort << "Some people join " << t->id << "!";
      } else {
        ss << " and encountered a group of people who can't join you because you control no solars!";
        sshort << "a group of people can't join!";
      }
      message_set = true;
    } else if (test < 0.3) {
      // additional ships
      research::data rbase;
      hm_t<string, float> prob;
      prob["scout"] = 0.3;
      prob["fighter"] = 2;
      prob["bomber"] = 1;
      prob["battleship"] = 0.1;
      prob["corsair"] = 0.2;
      prob["destroyer"] = 0.1;
      prob["voyager"] = 0.2;
      prob["freighter"] = 0.4;
      prob["colonizer"] = 0.6;

      list<combid> new_ships;
      for (auto m : prob) {
        int count = utility::random_normal(m.second, 0.2 * m.second);
        if (count > 0) {
          for (int j = 0; j < count; j++) {
            ship sh = rbase.build_ship(g->next_id(ship::class_id), m.first);
            new_ships.push_back(sh.id);
            sh.owner = s->owner;
            g->register_entity(ship::ptr(new ship(sh)));
          }
          break;
        }
      }

      if (new_ships.size()) {
        command c;
        c.action = fleet_action::idle;
        c.target = identifier::target_idle;

        if (g->allow_add_fleet(s->owner)) {
          g->generate_fleet(t->position, s->owner, c, new_ships);
          ss << " and encountered a band of " << new_ships.size() << " renegade ships who join your civilization.";
          sshort << new_ships.size() << " ships join!";
        } else {
          ss << " and encountered potential allies who could not join because you cannot control additional fleets.";
          sshort << new_ships.size() << " ships refused due to fleet limit!";
        }
        message_set = true;
      }
    } else if (test < 0.5) {
      // ship upgrade
      auto keys = utility::range_init<list<string>>(utility::hm_keys(upgrade::table()));
      for (auto u : s->upgrades) keys.remove(u);
      if (keys.size()) {
        string u = utility::uniform_sample(keys);
        s->upgrades.insert(u);
        ss << " and discovers an upgrade: " << u << "!";
        sshort << "upgrade: " << u << "!";
        message_set = true;
      }
    } else {
      // nothing
    }

    if (!message_set) {
      ss << " but found nothing of interest.";
      sshort << "nothing of interest.";
    }

    // log message to player
    g->log_message(s->id, ss.str(), sshort.str());

    // this solar can't be searched again
    t->was_discovered = true;
    g->get_fleet(s->fleet_id)->set_idle();
  };
  data[i.name] = i;

  // auto search
  i.name = interaction::auto_search;
  i.condition = target_condition(target_condition::neutral, solar::class_id);
  i.perform = [do_search](game_object::ptr self, game_object::ptr target, game_data *g) {
    do_search(self, target, g);

    // check ship has fleet
    if (!self->isa(ship::class_id)) return;
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    if (!s->has_fleet()) return;
    fleet::ptr f = g->get_fleet(s->fleet_id);

    // find new target
    target_condition cond(target_condition::neutral, solar::class_id);
    list<combid> test = g->search_targets_nophys(f->id, f->position, 300, cond.owned_by(f->owner));
    for (auto i = test.begin(); i != test.end(); i++)
      if (g->get_solar(*i)->was_discovered) test.erase(i--);
    if (test.empty()) return;

    // update fleet command
    f->com.action = interaction::auto_search;
    f->com.target = utility::uniform_sample(test);
    f->force_refresh = true;
  };
  data[i.name] = i;

  // space combat
  i.name = interaction::space_combat;
  i.condition = target_condition(target_condition::enemy, ship::class_id);
  i.perform = [](game_object::ptr self, game_object::ptr target, game_data *g) {
    output("interaction: space_combat: " + self->id + " targeting " + target->id);
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    ship::ptr t = utility::guaranteed_cast<ship>(target);

    if (s->load < s->stats[sskey::key::load_time]) return;

    g->log_ship_fire(s->id, t->id);

    output("space_combat: loaded");
    s->load = 0;
    if (t->evasion_check() < s->accuracy_check(t)) {
      output("space_combat: hit!");
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

    } else {
      output("space_combat: miss!");
    }
  };
  data[i.name] = i;

  i.name = interaction::search;
  i.condition = target_condition(target_condition::neutral, solar::class_id);
  i.perform = do_search;
  data[i.name] = i;

  // // turret combat
  // i.name = interaction::turret_combat;
  // i.condition = target_condition(target_condition::enemy, ship::class_id);
  // i.perform = [] (game_object::ptr self, game_object::ptr target, game_data *g){
  //   output("interaction: turret_combat: " + self -> id + " targeting " + target -> id);
  //   solar::ptr s = utility::guaranteed_cast<solar>(self);
  //   ship::ptr x = utility::guaranteed_cast<ship>(target);
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
  i.perform = [](game_object::ptr self, game_object::ptr target, game_data *g) {
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    solar::ptr t = utility::guaranteed_cast<solar>(target);

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
  i.perform = [](game_object::ptr self, game_object::ptr target, game_data *g) {
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    solar::ptr t = utility::guaranteed_cast<solar>(target);

    // if (s->ddata_int("passengers") == 0) return;

    // t -> population = s -> ddata_int("passengers");
    t->owner = s->owner;
    t->development[keywords::key_population] = 1;

    s->remove = true;
  };
  data[i.name] = i;

  // // pickup
  // i.name = interaction::pickup;
  // i.condition = target_condition(target_condition::owned, solar::class_id);
  // i.perform = [] (game_object::ptr self, game_object::ptr target, game_data *g){
  //   ship::ptr s = utility::guaranteed_cast<ship>(self);
  //   solar::ptr t = utility::guaranteed_cast<solar>(target);

  //   if (s -> ddata_int("passengers") > 0) return;

  //   int pickup = 1;
  //   int leave = 1;

  //   if (t -> population >= leave + pickup) {
  //     t -> population -= pickup;
  //     s -> dynamic_data["passengers"] = to_string(pickup);
  //   }
  // };
  // data[i.name] = i;

  // trade_to
  i.name = interaction::trade_to;
  i.condition = target_condition(target_condition::owned, solar::class_id);
  i.perform = [](game_object::ptr self, game_object::ptr target, game_data *g) {
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    solar::ptr t = utility::guaranteed_cast<solar>(target);
    if (!s->has_fleet()) return;
    fleet::ptr f = g->get_fleet(s->fleet_id);

    // first check if we still need to load resources
    if (!s->states.count("loaded")) {
      if (f->com.action == interaction::trade_to && g->entity_exists(f->com.source)) {
        game_object::ptr h = g->get_game_object(f->com.source);
        if (h->isa(solar::class_id) && h->owner == s->owner) {
          load_resources(s, utility::guaranteed_cast<solar>(h));
        }
      }
      return;
    }

    // unload resources
    for (auto v : keywords::resource) {
      t->resources[v] += s->cargo[v];
    }
    s->cargo = cost::res_t();
    s->states.erase("loaded");

    // check that we are the last ship in fleet unloading
    for (auto sid : f->ships) {
      ship::ptr sh = g->get_ship(sid);
      if (sh->states.count("loaded")) return;
    }

    // go back for more
    f->com.action = interaction::trade_from;
    f->com.target = f->com.origin;
    f->com.origin = target->id;
    f->force_refresh = true;
  };
  data[i.name] = i;

  // trade_from
  i.name = interaction::trade_from;
  i.condition = target_condition(target_condition::owned, solar::class_id);
  i.perform = [](game_object::ptr self, game_object::ptr target, game_data *g) {
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    solar::ptr t = utility::guaranteed_cast<solar>(target);
    if (!s->has_fleet()) return;
    if (s->states.count("loaded")) return;

    load_resources(s, t);

    // check that we are the last ship in fleet loading
    fleet::ptr f = g->get_fleet(s->fleet_id);
    for (auto sid : f->ships) {
      ship::ptr sh = g->get_ship(sid);
      if (!sh->states.count("loaded")) return;
    }

    // set target
    f->com.action = interaction::trade_to;
    f->com.target = f->com.origin;
    f->com.origin = target->id;
    f->force_refresh = true;
  };
  data[i.name] = i;

  // // terraform
  // i.name = terraform;
  // i.condition = target_condition(target_condition::neutral, solar::class_id);
  // i.perform = [] (game_object::ptr self, game_object::ptr target, game_data *g){
  //   ship::ptr s = utility::guaranteed_cast<ship>(self);
  //   solar::ptr t = utility::guaranteed_cast<solar>(target);

  //   if (s -> load < s -> stats[sskey::key::load_time]) return;
  //   s -> load = 0;

  //   t -> water = 1000;
  //   t -> space = 1000;
  //   t -> ecology = 1;

  //   if (s -> has_fleet()) g -> get_fleet(s -> fleet_id) -> set_idle();
  // };
  // data[i.name] = i;

  // hive support
  i.name = hive_support;
  i.perform = [](game_object::ptr self, game_object::ptr null_pointer, game_data *g) {
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    float strength = s->local_friends.size() / 40;
    for (auto sid : s->local_friends) {
      ship::ptr sh = g->get_ship(sid);
      sh->load += strength;
      sh->stats[sskey::key::evasion] = (1 + strength) * sh->base_stats.stats[sskey::key::evasion];
    }
  };
  data[i.name] = i;

  // splash
  i.name = splash;
  i.condition = target_condition(target_condition::enemy, ship::class_id);
  i.perform = [](game_object::ptr self, game_object::ptr target, game_data *g) {
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    ship::ptr t = utility::guaranteed_cast<ship>(target);

    float damage = 0.2 * s->stats[sskey::key::ship_damage];
    auto ns = g->entity_grid->search(t->position, 20);
    for (auto x : ns) {
      combid id = x.first;
      if (identifier::get_type(id) == ship::class_id) {
        ship::ptr t2 = g->get_ship(id);
        t2->receive_damage(g, s, damage);
      }
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
bool target_condition::valid_on(game_object::ptr p) {
  bool type_match = !(requires_target() && identifier::get_type(p->id) != what);
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
