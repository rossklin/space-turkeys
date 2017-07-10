#include <iostream>
#include <sstream>

#include "game_data.h"
#include "interaction.h"
#include "fleet.h"
#include "ship.h"
#include "utility.h"
#include "solar.h"
#include "upgrades.h"

using namespace std;
using namespace st3;

const class_t target_condition::no_target = "no target";
const string interaction::trade_to = "trade to";
const string interaction::trade_from = "trade from";
const string interaction::land = "land";
const string interaction::search = "search";
const string interaction::turret_combat = "turret combat";
const string interaction::space_combat = "space combat";
const string interaction::bombard = "bombard";
const string interaction::colonize = "colonize";
const string interaction::pickup = "pickup";
const string interaction::terraform = "terraform";
const string interaction::hive_support = "hive support";

const hm_t<string, interaction> &interaction::table() {
  static bool init = false;
  static hm_t<string, interaction> data;

  if (init) return data;
  interaction i;

  // land
  i.name = interaction::land;
  i.condition = target_condition(target_condition::owned, solar::class_id);
  i.perform = [] (game_object::ptr self, game_object::ptr target, game_data *g){
    cout << "interaction: land: " << self -> id << " targeting " << target -> id << endl;
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    solar::ptr t = utility::guaranteed_cast<solar>(target);
    cout << s -> id << " lands at " << t -> id << endl;

    // unset fleet
    if (s -> has_fleet()) {
      g -> get_fleet(s -> fleet_id) -> remove_ship(s -> id);
    }
    s -> fleet_id = identifier::source_none;

    // unload cargo
    t -> resource_storage.add(s -> cargo);
    s -> cargo = cost::res_t();
    s -> is_loaded = false;

    // add to solar's fleet
    s -> is_landed = true;
    t -> ships.insert(s -> id);
  };
  data[i.name] = i;

  // search
  i.name = interaction::search;
  i.condition = target_condition(target_condition::neutral, solar::class_id);
  i.perform = [] (game_object::ptr self, game_object::ptr target, game_data *g){
    cout << "interaction: search: " << self -> id << " targeting " << target -> id << endl;
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    solar::ptr t = utility::guaranteed_cast<solar>(target);
    stringstream ss;

    ss << "Your " << s -> ship_class << " searched " << t -> id;

    float test = utility::random_uniform();
    bool message_set = false;
    if (t -> was_discovered) {
      // nothing happens
    } else if (test < 0.1) {
      // random tech
      auto keys = utility::get_map_keys(research::data::table());
      research::data &level = g -> players[s -> owner].research_level;
      auto researched = level.researched();
      for (auto v : researched) keys.remove(v);

      if (keys.size() > 0) {
	string tech = utility::uniform_sample(keys);
	level.access(tech).level = 1;
	ss << " and discovered an ancient technology: " << tech << "!";
	message_set = true;
      }
    } else if (test < 0.2) {
      // population boost
      solar::ptr csol = g -> closest_solar(t -> position, s -> owner);
      if (csol) {
	float pop = fmax(utility::random_normal(500, 200), 10);
	csol -> population += pop;
	ss << " and encountered a group of " << int(pop) << " people who join your civilization at " << t -> id << "!";
      } else {
	ss << " and encountered a group of people who can't join you because you control no solars!";
      }
      message_set = true;
    } else if (test < 0.3) {
      // additional ships
      research::data rbase;
      hm_t<string, float> prob;
      prob["scout"] = 3;
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
	for (int j = 0; j < count; j++) {
	  ship sh = rbase.build_ship(m.first, t);
	  new_ships.push_back(sh.id);
	  sh.owner = s -> owner;
	  g -> add_entity(ship::ptr(new ship(sh)));
	}
      }

      if (new_ships.size()) {
	command c;
	c.action = fleet_action::idle;
	c.target = identifier::target_idle;

	g -> generate_fleet(t -> position, s -> owner, c, new_ships);

	ss << " and encountered a band of " << new_ships.size() << " renegade ships who join your civilization.";
	message_set = true;
      }
    } else if (test < 0.5) {
      // ship upgrade
      auto keys = utility::get_map_keys(upgrade::table());
      for (auto u : s -> upgrades) keys.remove(u);
      if (keys.size()) {
	string u = utility::uniform_sample(keys);
	s -> upgrades.insert(u);
	ss << " and discovers an upgrade: " << u << "!";
	message_set = true;
      }
    } else {
      // nothing
    }

    if (!message_set) {
      ss << " but found nothing of interest.";
    }

    // log message to player
    g -> players[s -> owner].log.push_back(ss.str());

    // this solar can't be searched again
    t -> was_discovered = true;
    g -> get_fleet(s -> fleet_id) -> set_idle();
  };
  data[i.name] = i;

  // space combat
  i.name = interaction::space_combat;
  i.condition = target_condition(target_condition::enemy, ship::class_id);
  i.perform = [] (game_object::ptr self, game_object::ptr target, game_data *g){
    cout << "interaction: space_combat: " << self -> id << " targeting " << target -> id << endl;
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    ship::ptr t = utility::guaranteed_cast<ship>(target);
      
    if (s -> load < s -> stats[sskey::key::load_time]) return;

    g -> log_ship_fire(s -> id, t -> id);

    cout << "space_combat: loaded" << endl;
    s -> load = 0;
    if (t -> evasion_check() < s -> accuracy_check(t)) {
      cout << "space_combat: hit!" << endl;
      float damage = 0;
      if (s -> stats[sskey::key::ship_damage] > 0) damage = utility::random_uniform(0, s -> stats[sskey::key::ship_damage]);
      t -> receive_damage(g, self, damage);
    }else{
      cout << "space_combat: miss!" << endl;
    }
  };
  data[i.name] = i;

  // turret combat
  i.name = interaction::turret_combat;
  i.condition = target_condition(target_condition::enemy, ship::class_id);
  i.perform = [] (game_object::ptr self, game_object::ptr target, game_data *g){
    cout << "interaction: turret_combat: " << self -> id << " targeting " << target -> id << endl;
    solar::ptr s = utility::guaranteed_cast<solar>(self);
    ship::ptr x = utility::guaranteed_cast<ship>(target);
    float d = utility::l2norm(s -> position - x -> position);

    for (auto &buf : s -> development){
      if (!buf.second.is_turret) continue;

      turret_t &t = buf.second.turret;

      // don't overdo it
      if (x -> remove) break;

      if (t.damage > 0 && t.load >= 1 && d <= t.range){
	t.load = 0;

	g -> log_ship_fire(s -> id, x -> id);
	
	if (x -> evasion_check() < t.accuracy_check(x)){
	  x -> receive_damage(g, s, utility::random_uniform(0, t.damage));
	}
      }
    }
  };
  data[i.name] = i;

  // bombard
  i.name = interaction::bombard;
  i.condition = target_condition(target_condition::enemy, solar::class_id);
  i.perform = [] (game_object::ptr self, game_object::ptr target, game_data *g){
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    solar::ptr t = utility::guaranteed_cast<solar>(target);

    if (s -> load < s -> stats[sskey::key::load_time]) return;

    // deal damage
    s -> load = 0;
    if (utility::random_uniform() < s -> stats[sskey::key::accuracy]){
      t -> receive_damage(s, utility::random_uniform(0, s -> stats[sskey::key::solar_damage]), g);
    }
  };
  data[i.name] = i;

  // colonize
  i.name = interaction::colonize;
  i.condition = target_condition(target_condition::neutral, solar::class_id);
  i.perform = [] (game_object::ptr self, game_object::ptr target, game_data *g){
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    solar::ptr t = utility::guaranteed_cast<solar>(target);

    if (s -> passengers == 0) return;
    
    t -> population = s -> passengers;
    t -> happiness = 1;
    t -> owner = s -> owner;
    t -> choice_data.allocation = cost::sector_allocation::base_allocation();

    s -> remove = true;
  };
  data[i.name] = i;

  // pickup
  i.name = interaction::pickup;
  i.condition = target_condition(target_condition::owned, solar::class_id);
  i.perform = [] (game_object::ptr self, game_object::ptr target, game_data *g){
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    solar::ptr t = utility::guaranteed_cast<solar>(target);

    if (s -> passengers > 0) return;

    int pickup = 1000;
    int leave = 1000;

    if (t -> population >= leave + pickup) {
      t -> population -= pickup;
      s -> passengers = pickup;
    }
  };
  data[i.name] = i;

  // trade_to
  i.name = interaction::trade_to;
  i.condition = target_condition(target_condition::owned, solar::class_id);
  i.perform = [] (game_object::ptr self, game_object::ptr target, game_data *g){
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    solar::ptr t = utility::guaranteed_cast<solar>(target);
    if (!s -> has_fleet()) return;
    if (!s -> is_loaded) return;

    // load resources
    for (auto v : keywords::resource) {
      t -> resource_storage[v] += s -> cargo[v];
    }
    s -> cargo = cost::res_t();
    s -> is_loaded = false;

    fleet::ptr f = g -> get_fleet(s -> fleet_id);

    // check that we are the last ship in fleet unloading
    for (auto sid : f -> ships) {
      ship::ptr sh = g -> get_ship(sid);
      if (sh -> is_loaded) return;
    }
    
    // go back for more
    f -> com.action = interaction::trade_from;
    f -> com.target = f -> com.origin;
    f -> com.origin = target -> id;
  };
  data[i.name] = i;

  // trade_from
  i.name = interaction::trade_from;
  i.condition = target_condition(target_condition::owned, solar::class_id);
  i.perform = [] (game_object::ptr self, game_object::ptr target, game_data *g){
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    solar::ptr t = utility::guaranteed_cast<solar>(target);
    if (!s -> has_fleet()) return;
    if (s -> is_loaded) return;

    // load resources
    float total = t -> resource_storage.count();
    float ratio = fmax(s -> stats[sskey::key::cargo_capacity] / total, 1);
    cost::res_t move = t -> resource_storage;
    move.scale(ratio);
    s -> cargo.add(move);
    move.scale(-1);
    t -> resource_storage.add(move);
    s -> is_loaded = true;

    // check that we are the last ship in fleet loading
    fleet::ptr f = g -> get_fleet(s -> fleet_id);
    for (auto sid : f -> ships) {
      ship::ptr sh = g -> get_ship(sid);
      if (!sh -> is_loaded) return;
    }

    // set target
    f -> com.action = interaction::trade_to;
    f -> com.target = f -> com.origin;
    f -> com.origin = target -> id;
  };
  data[i.name] = i;

  // terraform
  i.name = terraform;
  i.condition = target_condition(target_condition::neutral, solar::class_id);
  i.perform = [] (game_object::ptr self, game_object::ptr target, game_data *g){
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    solar::ptr t = utility::guaranteed_cast<solar>(target);

    if (s -> load < s -> stats[sskey::key::load_time]) return;
    s -> load = 0;

    t -> water = 1000;
    t -> space = 1000;
    t -> ecology = 1;
  };
  data[i.name] = i;

  // terraform
  i.name = hive_support;
  i.perform = [] (game_object::ptr self, game_object::ptr null_pointer, game_data *g) {
    ship::ptr s = utility::guaranteed_cast<ship>(self);
    for (auto sid : s -> local_friends) g -> get_ship(sid) -> load++;
  };
  data[i.name] = i;

  init = true;
  return data;
}

bool target_condition::get_alignment(idtype t, idtype s){
  if (t == game_object::neutral_owner) return target_condition::neutral;
  return s == t ? target_condition::owned : target_condition::enemy;
};

// target condition
target_condition::target_condition(){}

target_condition::target_condition(sint a, class_t w) : alignment(a), what(w), owner(game_object::neutral_owner) {}

target_condition target_condition::owned_by(idtype o){
  target_condition t = *this;
  t.owner = o;
  return t;
}

bool target_condition::requires_target(){
  return what != no_target;
}

// interaction
bool target_condition::valid_on(game_object::ptr p){
  bool type_match = !(requires_target() && identifier::get_type(p -> id) != what);
  sint aligned = 0;
  if (owner == p -> owner) {
    aligned = target_condition::owned;
  }else if (p -> owner == game_object::neutral_owner) {
    aligned = target_condition::neutral;
  }else {
    aligned = target_condition::enemy;
  }
  bool align_match = aligned & alignment;

  return type_match && align_match;
}
