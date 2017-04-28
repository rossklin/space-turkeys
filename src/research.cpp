#include <vector>

#include "research.h"
#include "cost.h"
#include "utility.h"
#include "upgrades.h"
#include "fleet.h"
#include "choice.h"

using namespace std;
using namespace st3;
using namespace research;
using namespace cost;

ship ship_template(string k){
  static bool init = false;
  static cost::ship_allocation<ship> buf;

  if (!init){    
    init = true;

    ship s, a;
    s.base_stats.speed = 1;
    s.base_stats.vision = 50;
    s.base_stats.hp = 1;
    s.base_stats.ship_damage = 0;
    s.base_stats.solar_damage = 0;
    s.base_stats.accuracy = 0;
    s.base_stats.interaction_radius = 20;
    s.fleet_id = identifier::source_none;
    s.ship_class = "";
    s.remove = false;
    s.base_stats.load_time = 100;
    s.load = 0;
    
    auto add_with_class = [&buf] (ship s, string c){
      s.ship_class = c;
      buf[c] = s;
    };

    a = s;
    a.base_stats.speed = 2;
    a.base_stats.vision = 100;
    a.base_stats.ship_damage = 0.1;
    a.base_stats.accuracy = 0.3;
    a.upgrades.insert(fleet_action::space_combat);
    add_with_class(a, keywords::key_scout);

    a = s;
    a.base_stats.hp = 2;
    a.base_stats.ship_damage = 1;
    a.base_stats.solar_damage = 0.1;
    a.base_stats.accuracy = 0.7;
    a.base_stats.interaction_radius = 40;
    a.base_stats.load_time = 30;
    a.upgrades.insert(fleet_action::space_combat);
    a.upgrades.insert(fleet_action::bombard);
    add_with_class(a, keywords::key_fighter);

    a = s;
    a.base_stats.solar_damage = 5;
    a.base_stats.accuracy = 0.8;
    a.upgrades.insert(fleet_action::bombard);
    add_with_class(a, keywords::key_bomber);

    a = s;
    a.base_stats.speed = 0.5;
    a.base_stats.hp = 2;
    a.upgrades.insert(fleet_action::colonize);
    add_with_class(a, keywords::key_colonizer);

    buf.confirm_content(cost::keywords::ship);
  }

  return buf[k];
}

turret turret_template(string k){
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

  return buf[k];
}

data::data(){
  // todo: build research tree
}

list<string> data::available() {
  list<string> res;

  for (auto x : tree){
    tech t = x.second;
    if (researched.count(t.name)) continue;
    if (t.cost > accumulated) continue;
    if (t.req_facility_level > facility_level) continue;
    if ((t.depends - researched).size() > 0) continue;
    res.push_back(t.name);
  }

  return res;
}

ship data::build_ship(string c){
  ship s = ship_template(c);

  // add upgrades from research tree
  for (auto t : researched) s.upgrades += tree[t].ship_upgrades[c];

  // evaluate upgrades
  auto utab = upgrade::table();
  for (auto u : s.upgrades) s.base_stats += utab[u].modify;
  s.current_stats = s.base_stats;

  // apply id  
  static int idc = 0;
  s.id = identifier::make(ship::class_id, idc++);

  return s;
}

turret data::build_turret(string v){
  turret t = turret_template(v);

  // todo: apply research boosts

  return t;
}
