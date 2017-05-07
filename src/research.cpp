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
    s.base_stats.load_time = 50;
    s.fleet_id = identifier::source_none;
    s.ship_class = "";
    s.remove = false;
    s.load = 0;
    s.upgrades.insert(interaction::land);
    s.depends_tech = "";
    s.depends_facility_level = 0;
    s.cargo_capacity = 0;
    
    auto add_with_class = [] (ship s, string c){
      s.ship_class = c;
      buf[c] = s;
    };

    a = s;
    a.base_stats.speed = 2;
    a.base_stats.vision = 100;
    a.base_stats.ship_damage = 0.1;
    a.base_stats.accuracy = 0.3;
    a.upgrades.insert(interaction::space_combat);
    add_with_class(a, keywords::key_scout);

    a = s;
    a.base_stats.hp = 2;
    a.base_stats.ship_damage = 1;
    a.base_stats.solar_damage = 0.1;
    a.base_stats.accuracy = 0.7;
    a.base_stats.interaction_radius = 40;
    a.base_stats.load_time = 20;
    a.upgrades.insert(interaction::space_combat);
    a.upgrades.insert(interaction::bombard);
    a.depends_facility_level = 1;
    a.depends_tech = "ship armor";
    add_with_class(a, keywords::key_fighter);

    a = s;
    a.base_stats.solar_damage = 5;
    a.base_stats.accuracy = 0.8;
    a.upgrades.insert(interaction::bombard);
    a.depends_facility_level = 2;
    a.depends_tech = "ship weapons";
    add_with_class(a, keywords::key_bomber);

    a = s;
    a.base_stats.speed = 0.5;
    a.base_stats.hp = 2;
    a.upgrades.insert(interaction::colonize);
    a.depends_facility_level = 1;
    add_with_class(a, keywords::key_colonizer);

    a = s;
    a.base_stats.speed = 1;
    a.base_stats.hp = 1;
    a.upgrades.insert(interaction::trade_to);
    a.upgrades.insert(interaction::trade_from);
    a.depends_facility_level = 1;
    a.cargo_capacity = 10;
    add_with_class(a, keywords::key_freighter);

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
    x.load_time = 30;
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
  accumulated = 0;
  facility_level = 0;

  tech t;

  // upgrade "ship armor"
  t.name = "ship armor";
  t.cost = 1;
  t.req_facility_level = 1;
  t.ship_upgrades[research::upgrade_all_ships] = {"ship armor"};
  tree[t.name] = t;

  // upgrade "ship speed"
  t.name = "ship speed";
  t.cost = 1;
  t.req_facility_level = 1;
  t.ship_upgrades[research::upgrade_all_ships] = {"ship speed"};
  tree[t.name] = t;

  // upgrade "ship weapons"
  t.name = "ship weapons";
  t.cost = 2;
  t.req_facility_level = 1;
  t.depends = {"ship armor", "ship speed"};
  t.ship_upgrades[research::upgrade_all_ships] = {"ship weapons"};
  tree[t.name] = t;
  
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

void data::repair_ship(ship &s) {
  ship ref = ship_template(s.ship_class);

  s.angle = utility::random_uniform(0, 2 * M_PI);
  
  // add upgrades from research tree
  for (auto t : researched) {
    s.upgrades += tree[t].ship_upgrades[s.class_id];
    s.upgrades += tree[t].ship_upgrades[research::upgrade_all_ships];
  }

  // evaluate upgrades
  auto utab = upgrade::table();
  s.base_stats = ref.base_stats;
  for (auto u : s.upgrades) s.base_stats += utab[u].modify;
  s.current_stats = s.base_stats;
}

ship data::build_ship(string c){
  ship s = ship_template(c);

  // apply id  
  static int idc = 0;
  s.id = identifier::make(ship::class_id, idc++);

  // apply upgrades
  repair_ship(s);

  return s;
}

turret data::build_turret(string v){
  turret t = turret_template(v);

  // todo: apply research boosts

  return t;
}

bool data::can_build_ship(string v, solar::ptr sol){
  int facility = sol -> sector[cost::keywords::key_military];
  ship s = ship_template(v);
  if (s.depends_facility_level > facility) return false;
  if (s.depends_tech.length() > 0 && !researched.count(s.depends_tech)) return false;
  for (auto v : s.upgrades) {
    upgrade u = upgrade::table()[v];
    if (u.depends && !u.depends(sol)) return false;
  }
  return true;
}

hm_t<string,choice::c_solar> data::solar_template_table(solar sol){
  static hm_t<string, choice::c_solar> data;
  using namespace cost;

  // basic pop growth
  choice::c_solar empty;
  choice::c_solar x = empty;
  x.allocation[keywords::key_culture] = 1;
  data["basic growth"] = x;

  // culture growth
  x = empty;
  x.allocation[keywords::key_culture] = 1;
  x.allocation[keywords::key_expansion] = 1;    
  x.expansion[keywords::key_culture] = 1;
  sol.autofill_mining(x);
  data["culture growth"] = x;

  // mining colony
  x = empty;
  x.allocation[keywords::key_culture] = 1;
  x.allocation[keywords::key_expansion] = 1;
  x.allocation[keywords::key_mining] = 3;
  x.expansion[keywords::key_mining] = 1;
  x.mining[keywords::key_metals] = 1;
  x.mining[keywords::key_gases] = 1;
  x.mining[keywords::key_organics] = 1;
  data["mining colony"] = x;

  // military expansion
  x = empty;
  x.allocation[keywords::key_culture] = 1;
  x.allocation[keywords::key_expansion] = 1;
  x.allocation[keywords::key_military] = 3;
    
  x.expansion[keywords::key_culture] = 1;
  x.expansion[keywords::key_mining] = 1;
  x.expansion[keywords::key_military] = 3;

  if (can_build_ship(keywords::key_bomber, &sol)) x.military.c_ship[keywords::key_bomber] = 1;
  if (can_build_ship(keywords::key_fighter, &sol)) x.military.c_ship[keywords::key_fighter] = 2;
  x.military.c_turret[keywords::key_rocket_turret] = 2;

  sol.autofill_mining(x);
  data["military expansion"] = x;

  // research & development
  x = empty;
  x.allocation[keywords::key_culture] = 1;
  x.allocation[keywords::key_expansion] = 1;
  x.allocation[keywords::key_research] = 2;
  x.expansion[keywords::key_research] = 2;
  x.expansion[keywords::key_culture] = 1;
  sol.autofill_mining(x);
  data["research"] = x;
  
  return data;
}
