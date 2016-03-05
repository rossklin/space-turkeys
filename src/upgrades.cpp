#include "interaction.h"
#include "upgrades.h"
#include "fleet.h"

using namespace std;
using namespace st3;

upgrade compile_upgrade(interaction i) {
  upgrade u;
  u.inter.push_back(i);
  u.modify = [] (ship::ptr) {};
  return u;
}

upgrade upgrade::table(string k){
  static bool init = false;
  static hm_t<string, upgrade> data;

  if (!init){
    init = true;

    interaction i;

    // space combat
    i.name = fleet::action::space_combat;
    i.condition = target_condition(target_condition::enemy, identifier::ship);
    i.perform = [] (game_object::ptr self, game_object::ptr target, game_data *context){
      ship::ptr s = utility::guaranteed_cast<ship::ptr>(self);
      ship::ptr t = utility::guaranteed_cast<ship::ptr>(target);

      if (s -> load < s -> current_stats.load_time) return;

      s -> load = 0;
      if (utility::random_uniform() < s -> current_stats.accuracy){
	t -> receive_damage(s, t, utility::random_uniform(0, s -> current_stats.ship_damage));
      }
    };
    data[space_combat] = compile_upgrade(i);

    // bombard
    i.name = fleet::action::bombard;
    i.condition = target_condition(target_condition::enemy, identifier::solar);
    i.perform = [] (game_object::ptr self, game_object::ptr target, game_data *context){
      ship::ptr s = utility::guaranteed_cast<ship::ptr>(self);
      solar::ptr t = utility::guaranteed_cast<solar::ptr>(target);

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
    data[bombard] = compile_upgrade(i);

    // colonize
    i.name = "colonize";
    i.condition = target_condition(target_condition::neutral, identifier::solar);
    i.perform = [] (game_object::ptr self, game_object::ptr target, game_data *g){
      ship::ptr s = utility::guaranteed_cast<ship::ptr>(self);
      solar::ptr t = utility::guaranteed_cast<solar::ptr>(target);

      // check if solar already colonized
      if (t -> owner == s -> owner){
	return;
      }

      // check colonization
      if (t -> owner == game_object::neutral_owner && !t -> has_defense()){
	t -> colonization_attempts[s -> owner] = s -> id;
      }
    };
    data[colonize] = compile_upgrade(i);
  }

  return data[k];
}
