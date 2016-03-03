#include <vector>

#include "research.h"
#include "cost.h"

using namespace std;
using namespace st3;
using namespace research;
using namespace cost;

cost::ship_allocation<ship>& research::ship_templates(){
  static bool init = false;
  static cost::ship_allocation<ship> buf;

  if (!init){    
    init = true;

    interaction space_combat;
    space_combat.name = "space combat";
    space_combat.condition = ineraction::target_condition(identifier::ship, target_condition::enemy);
    space_combat.perform = [] (game_object::ptr self, game_object::ptr target, game_data *context){
      ship::ptr s = utility::attempt_cast<ship::ptr>(self);
      ship::ptr t = utility::attempt_cast<ship::ptr>(target);

      if (s -> load < s -> load_time) return;

      s -> load = 0;
      if (utility::random_uniform() < s -> accuracy){
	t -> receive_damage(s, t, utility::random_uniform(0, s -> ship_damage));
      }
    };

    interaction bombard;
    bombard.name = "bombard";
    bombard.condition = ineraction::target_condition(identifier::solar, target_condition::enemy);
    bombard.perform = [] (game_object::ptr self, game_object::ptr target, game_data *context){
      ship::ptr s = utility::attempt_cast<ship::ptr>(self);
      solar::ptr t = utility::attempt_cast<solar::ptr>(target);

      if (s -> load < s -> load_time) return;
      
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
      if (utility::random_uniform() < s -> accuracy){
	t -> damage_taken[s -> owner] += utility::random_uniform(0, damage);
      }
    };
    
    interaction colonize;
    colonize.name = "colonize";
    colonize.condition = target_condition(identifier::solar, target_condition::neutral);
    colonize.perform = [] (game_object::ptr self, game_object::ptr target, game_data *g){
      ship::ptr s = utility::attempt_cast<ship::ptr>(self);
      solar::ptr t = utility::attempt_cast<solar::ptr>(target);

      // check if solar already colonized
      if (t -> owner == s.owner){
	return;
      }

      // check colonization
      if (t -> owner == game_object::neutral_owner && !t -> has_defense()){
	t -> colonization_attempts[s -> owner] = s -> id;
      }
    }
    
    ship s, a;
    s.speed = 1;
    s.vision = 50;
    s.hp = 1;
    s.interaction_radius = 10;
    s.fleet_id = -1;
    s.ship_class = "";
    s.remove = false;
    s.load_time = 100;
    s.load = 0;

    a = s;
    a.speed = 2;
    a.vision = 100;
    a.ship_class = keywords::key_scout;
    a.interaction_list[tc_enemy_ship] = fleet::action::combat;
    a.ship_interaction[fleet::action::combat] = default_combat(0.2, 0.3);
    buf[a.ship_class] = a;

    a = s;
    a.hp = 2;
    a.interaction_radius = 20;
    a.ship_class = keywords::key_fighter;
    a.load_time = 30;
    a.interaction_list[tc_enemy_ship] = fleet::action::combat;
    a.interaction_list[tc_enemy_solar] = fleet::action::combat;
    a.ship_interaction[fleet::action::combat] = default_combat(0.7, 2);
    a.solar_interaction[fleet::action::combat] = default_combat(0.7, 1);
    buf[a.ship_class] = a;

    a = s;
    a.ship_class = keywords::key_bomber;
    a.damage_solar = 5;
    a.accuracy = 0.8;
    buf[a.ship_class] = a;

    a = s;
    a.speed = 0.5;
    a.hp = 2;
    a.ship_class = keywords::key_colonizer;
    buf[a.ship_class] = a;

    buf.confirm_content(cost::keywords::ship);
  }

  return buf;
}

cost::turret_allocation<turret> &research::turret_templates(){
  static bool init = false;
  static cost::turret_allocation<turret> buf;

  if (!init){
    init = true;
    turret x,a;

    x.range = 50;
    x.hp = 1;
    x.vision = 100;
    x.damage = 1;
    x.accuracy = 0.5;
    x.load_time = 100;
    x.load = 0;

    a = x;
    a.turret_class = cost::keywords::key_rocket_turret;
    buf[a.turret_class] = a;

    a = x;
    a.damage = 0;
    a.vision = 200;
    a.turret_class = cost::keywords::key_radar_turret;
    buf[a.turret_class] = a;

    buf.confirm_content(cost::keywords::turret);
  }

  return buf;
}

data::data(){
  x = "research level 0";
}

ship data::build_ship(string c){
  ship s = ship_templates()[c];

  // todo: apply research boosts

  return s;
}

turret data::build_turret(string v){
  turret t = turret_templates()[v];

  // todo: apply research boosts

  return t;
}

void data::colonize(solar::solar *s){
  s -> population = 100;
  s -> happiness = 1;
}

int data::colonizer_population(){
  return 100;
}

void data::develope(float d){
  x = "research level " + to_string(d);
}
