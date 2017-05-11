#include <vector>
#include <rapidjson/document.h>

#include "research.h"
#include "cost.h"
#include "utility.h"
#include "upgrades.h"
#include "fleet.h"
#include "choice.h"
#include "ship.h"

using namespace std;
using namespace st3;
using namespace research;
using namespace cost;

data::data(){  
  accumulated = 0;
  facility_level = 0;
}

hm_t<string, tech>& data::table(){
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

    tree[a.name] = a;
  }
  
  init = true;
  return tree;
}

list<string> data::available() {
  list<string> res;

  for (auto x : table()){
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
  ship ref(ship_template(s.ship_class));

  s.angle = utility::random_uniform(0, 2 * M_PI);
  
  // add upgrades from research tree
  auto &rtab = table();
  for (auto t : researched) {
    s.upgrades += rtab[t].ship_upgrades[s.class_id];
    s.upgrades += rtab[t].ship_upgrades[research::upgrade_all_ships];
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

  // TODO: does assignment from parent class work?
  s = s.base_stats;
}

ship data::build_ship(string c, solar::ptr sol){
  ship s = ship::table().at(c);

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
