#include <vector>
#include <rapidjson/document.h>

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

ship ship_template(string k) {
  static bool init = false;
  static cost::ship_allocation<ship> buf;

  if (!init){    
    init = true;
    auto doc = utility::get_json("ship");

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
    s.passengers = 0;

    // read ships from json structure
    for (auto &x : doc) {
      a = s;
      if (x.value.HasMember("speed")) a.speed = x.value["speed"].GetFloat();
      if (x.value.HasMember("vision")) a.vision = x.value["vision"].GetFloat();
      if (x.value.HasMember("hp")) a.hp = x.value["hp"].GetFloat();
      if (x.value.HasMember("ship_damage")) a.ship_damage = x.value["ship_damage"].GetFloat();
      if (x.value.HasMember("solar_damage")) a.solar_damage = x.value["solar_damage"].GetFloat();
      if (x.value.HasMember("accuracy")) a.accuracy = x.value["accuracy"].GetFloat();
      if (x.value.HasMember("interaction_radius")) a.interaction_radius = x.value["interaction_radius"].GetFloat();
      if (x.value.HasMember("load_time")) a.load_time = x.value["load_time"].GetFloat();
      if (x.value.HasMember("cargo_capacity")) a.cargo_capacity = x.value["cargo_capacity"].GetFloat();
      if (x.value.HasMember("depends_tech")) a.depends_tech = x.value["depends_tech"].GetString();
      if (x.value.HasMember("depends_facility_level")) a.depends_facility_level = x.value["depends_facility_level"].GetInt();
      
      a.ship_class = x.name.GetString();

      if (x.value.HasMember("upgrades")) {
	if (!x.value["upgrades"].IsArray()) {
	  throw runtime_error("Invalid upgrades array in ship_data.json!");
	}
	
	for (auto &u : x.value["upgrades"]) s.upgrades.insert(u.GetString());
      }
      
      a.fleet_id = identifier::source_none;
      a.remove = false;
      a.load = 0;
      buf[a.ship_class] = a;
    }

    buf.confirm_content(cost::keywords::ship);
  }

  return buf[k];
}

data::data(){  
  accumulated = 0;
  facility_level = 0;
}

hm_t<string, tech>& data::get_tree(){
  static hm_t<std::string, tech> tree;
  static bool init = false;

  if (init) return tree;

  tech t, a;
  t.cost = 1;
  t.req_facility_level = 1;

  auto doc = utility::get_json("research");

  for (auto &x : doc) {
    a = t;
    a.name = x.name.GetString();

    if (x.value.HasMember("cost")) a.cost = x.value["cost"].GetFloat();
    if (x.value.HasMember("req_facility_level")) a.req_facility_level = x.value["req_facility_level"].GetFloat();

    if (x.value.HasMember("depends")) {
      if (!x.value["depends"].IsArray()) {
	throw runtime_error("Error: load research data: depends: not an array!");
      }
      
      for (auto &d : x.value["depends"]) {
	a.depends.insert(d.GetString());
      }
    }

    if (x.value.HasMember("ship upgrades")) {
      for (auto &u : x.value["ship upgrades"]) {
	string ship_class = u.name.GetString();
	for (auto &v : u.value) a.ship_upgrades[ship_class].insert(v.GetString());
      }
    }
  }
  
  init = true;
  return tree;
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

void data::repair_ship(ship &s, solar::ptr sol) {
  ship ref = ship_template(s.ship_class);

  s.angle = utility::random_uniform(0, 2 * M_PI);
  
  // add upgrades from research tree
  for (auto t : researched) {
    s.upgrades += tree[t].ship_upgrades[s.class_id];
    s.upgrades += tree[t].ship_upgrades[research::upgrade_all_ships];
  }

  // add upgrades from solar facilities
  for (auto x : sol -> development.facilities) {
    facility_object t = x.second;
    s.upgrades += t.ship_upgrades[s.class_id];
    s.upgrades += t.ship_upgrades[research::upgrade_all_ships];
  }

  // evaluate upgrades
  auto utab = upgrade::table();
  s.base_stats = ref.base_stats;
  for (auto u : s.upgrades) s.base_stats += utab[u].modify;
  s.current_stats = s.base_stats;
}

ship data::build_ship(string c, solar::ptr sol){
  ship s = ship_template(c);

  // apply id  
  static int idc = 0;
  s.id = identifier::make(ship::class_id, idc++);

  // apply upgrades
  repair_ship(s);

  return s;
}

bool data::can_build_ship(string v, solar::ptr sol){
  int facility = sol -> development.facilities[keywords::key_military].level;
  ship s = ship_template(v);
  if (s.depends_facility_level > facility) return false;
  if (s.depends_tech.length() > 0 && !researched.count(s.depends_tech)) return false;
  for (auto v : s.upgrades) {
    upgrade u = upgrade::table()[v];
    if (u.depends && !u.depends(sol)) return false;
  }
  return true;
}

hm_t<string,choice::c_solar> data::solar_template_table(solar::ptr sol){
  static hm_t<string, choice::c_solar> data;
  using namespace cost;

  // basic pop growth
  choice::c_solar empty;
  choice::c_solar x = empty;
  x.allocation[keywords::key_culture] = 1;
  data["basic growth"] = x;

  // culture growth
  x = empty;
  x.allocation[keywords::key_culture] = 3;
  x.allocation[keywords::key_expansion] = 1;
  x.allocation[keywords::key_mining] = 1;
  data["culture growth"] = x;

  // mining colony
  x = empty;
  x.allocation[keywords::key_culture] = 1;
  x.allocation[keywords::key_expansion] = 1;
  x.allocation[keywords::key_mining] = 3;
  data["mining colony"] = x;

  // military expansion
  x = empty;
  x.allocation[keywords::key_culture] = 1;
  x.allocation[keywords::key_expansion] = 1;
  x.allocation[keywords::key_military] = 3;
  x.allocation[keywords::key_mining] = 3;

  if (can_build_ship(keywords::key_bomber, &sol)) x.military[keywords::key_bomber] = 1;
  if (can_build_ship(keywords::key_fighter, &sol)) x.military[keywords::key_fighter] = 2;

  data["military expansion"] = x;

  // research & development
  x = empty;
  x.allocation[keywords::key_culture] = 1;
  x.allocation[keywords::key_expansion] = 1;
  x.allocation[keywords::key_research] = 1;
  x.allocation[keywords::key_mining] = 1;
  data["research"] = x;
  
  return data;
}
