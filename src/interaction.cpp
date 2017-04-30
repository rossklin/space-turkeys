#include <iostream>

#include "game_data.h"
#include "interaction.h"
#include "fleet.h"
#include "ship.h"
#include "utility.h"
#include "solar.h"

using namespace std;
using namespace st3;

const class_t target_condition::no_target = "no target";
const string interaction::land = "land";
const string interaction::turret_combat = "turret combat";
const string interaction::space_combat = "space combat";
const string interaction::bombard = "bombard";
const string interaction::colonize = "colonize";

hm_t<string, interaction> &interaction::table() {
  static bool init = false;
  static hm_t<string, interaction> data;

  if (!init){
    init = true;

    interaction i;

    // land
    i.name = interaction::land;
    i.condition = target_condition(target_condition::owned, solar::class_id);
    i.perform = [] (game_object::ptr self, game_object::ptr target, game_data *g){
      cout << "interaction: land: " << self -> id << " targeting " << target -> id << endl;
      ship::ptr s = utility::guaranteed_cast<ship>(self);
      solar::ptr t = utility::guaranteed_cast<solar>(target);

      cout << s -> id << " lands at " << t -> id << endl;
      s -> fleet_id = identifier::source_none;
      g -> get_fleet(s -> fleet_id) -> remove_ship(s -> id);
      t -> ships.insert(s -> id);
      g -> players[t -> owner].research_level.repair_ship(*s);
    };
    data[i.name] = i;

    // space combat
    i.name = interaction::space_combat;
    i.condition = target_condition(target_condition::enemy, ship::class_id);
    i.perform = [] (game_object::ptr self, game_object::ptr target, game_data *g){
      cout << "interaction: space_combat: " << self -> id << " targeting " << target -> id << endl;
      ship::ptr s = utility::guaranteed_cast<ship>(self);
      ship::ptr t = utility::guaranteed_cast<ship>(target);
      
      if (s -> load < s -> current_stats.load_time) return;

      cout << "space_combat: loaded" << endl;
      s -> load = 0;
      if (utility::random_uniform() < s -> current_stats.accuracy){
      	cout << "space_combat: hit!" << endl;
      	t -> receive_damage(self, utility::random_uniform(0, s -> current_stats.ship_damage));
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

      for (auto &t : s -> turrets){
	if (t.damage > 0 && t.load >= t.load_time && d <= t.range){
	  t.load = 0;
	
	  if (utility::random_uniform() < t.accuracy){
	    x -> receive_damage(s, utility::random_uniform(0, t.damage));
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

      if (s -> load < s -> current_stats.load_time) return;

      // deal damage
      s -> load = 0;
      if (utility::random_uniform() < s -> current_stats.accuracy){
  	t -> receive_damage(s, utility::random_uniform(0, s -> current_stats.solar_damage), g);
      }
    };
    data[i.name] = i;

    // colonize
    i.name = interaction::colonize;
    i.condition = target_condition(target_condition::neutral, solar::class_id);
    i.perform = [] (game_object::ptr self, game_object::ptr target, game_data *g){
      ship::ptr s = utility::guaranteed_cast<ship>(self);
      solar::ptr t = utility::guaranteed_cast<solar>(target);
      
      // todo: let ships carry colonists
      t -> population = 100;
      t -> happiness = 1;
      auto ctab = choice::c_solar::template_table();
      t -> choice_data = ctab["culture growth"];
    };
    data[i.name] = i;
  }

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
