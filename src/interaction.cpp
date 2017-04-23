#include <iostream>

#include "interaction.h"
#include "fleet.h"
#include "ship.h"
#include "utility.h"
#include "solar.h"

using namespace std;
using namespace st3;

const class_t target_condition::no_target = "no target";

hm_t<string, interaction> &interaction::table() {
  static bool init = false;
  static hm_t<string, interaction> data;

  if (!init){
    init = true;

    interaction i;

    // space combat
    i.name = fleet_action::space_combat;
    i.condition = target_condition(target_condition::enemy, ship::class_id, fleet::class_id);
    i.perform = [] (game_object::ptr self, game_object::ptr target){
      cout << "interaction: space_combat: " << self -> id << " targeting " << target -> id << endl;
      ship::ptr s = utility::guaranteed_cast<ship>(self);
      ship::ptr t = utility::guaranteed_cast<ship>(target);
      
      // todo: if the function returns here, how does it manage to
      // damage the vtable for game_object for self, but not for
      // target?
      if (s -> load < s -> current_stats.load_time) return;

      cout << "space_combat: loaded" << endl;
      s -> load = 0;
      if (utility::random_uniform() < s -> current_stats.accuracy){
	cout << "space_combat: hit!" << endl;
	t -> receive_damage(s, utility::random_uniform(0, s -> current_stats.ship_damage));
      }else{
	cout << "space_combat: miss!" << endl;
      }
    };
    data[i.name] = i;

    // bombard
    i.name = fleet_action::bombard;
    i.condition = target_condition(target_condition::enemy, solar::class_id);
    i.perform = [] (game_object::ptr self, game_object::ptr target){
      ship::ptr s = utility::guaranteed_cast<ship>(self);
      solar::ptr t = utility::guaranteed_cast<solar>(target);

      if (s -> load < s -> current_stats.load_time) return;
      
      // check if solar already captured
      if (t -> owner == s -> owner){
	return;
      }

      // check if defenses already destroyed
      if (t -> owner == -1 && !t -> has_defense()){
	return;
      }

      // deal damage
      s -> load = 0;
      if (utility::random_uniform() < s -> current_stats.accuracy){
	t -> damage_taken[s -> owner] += utility::random_uniform(0, s -> current_stats.solar_damage);
      }
    };
    data[i.name] = i;

    // colonize
    i.name = "colonize";
    i.condition = target_condition(target_condition::neutral, solar::class_id);
    i.perform = [] (game_object::ptr self, game_object::ptr target){
      ship::ptr s = utility::guaranteed_cast<ship>(self);
      solar::ptr t = utility::guaranteed_cast<solar>(target);

      // check if solar already colonized
      if (t -> owner == s -> owner){
	return;
      }

      // check colonization
      if (t -> owner == game_object::neutral_owner && !t -> has_defense()){
	t -> colonization_attempts[s -> owner] = s -> id;
      }
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

target_condition::target_condition(sint a, class_t w, class_t m) : alignment(a), what(w), owner(game_object::neutral_owner) {
  macro_target = m.empty() ? w : m;
}

target_condition target_condition::owned_by(idtype o){
  target_condition t = *this;
  t.owner = o;
  return t;
}

bool target_condition::requires_target(){
  return what != no_target;
}

bool interaction::macro_valid(target_condition c, game_object::ptr p){
  c.what = c.macro_target;
  return valid(c, p);
}

// interaction
bool interaction::valid(target_condition c, game_object::ptr p){
  bool type_match = !(c.requires_target() && identifier::get_type(p -> id) != c.what);
  sint aligned = 0;
  if (c.owner == p -> owner) {
    aligned = target_condition::owned;
  }else if (p -> owner == game_object::neutral_owner) {
    aligned = target_condition::neutral;
  }else {
    aligned = target_condition::enemy;
  }
  bool align_match = aligned & c.alignment;

  return type_match && align_match;
}
