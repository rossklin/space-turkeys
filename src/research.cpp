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

const hm_t<string, tech>& data::table(){
  static hm_t<std::string, tech> tree;
  static bool init = false;

  if (init) return tree;

  tech t, a;
  t.cost = 1;
  t.req_facility_level = 1;

  rapidjson::Document *docp = utility::get_json("research");
  auto &doc = (*docp)["research"];
  
  for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); i++) {
    a = t;
    a.name = i -> name.GetString();

    if (i -> value.HasMember("cost")) a.cost = i -> value["cost"].GetDouble();
    if (i -> value.HasMember("req_facility_level")) a.req_facility_level = i -> value["req_facility_level"].GetDouble();

    if (i -> value.HasMember("depends")) {
      if (!i -> value["depends"].IsArray()) {
	throw runtime_error("Error: load research data: depends: not an array!");
      }

      auto &depends = i -> value["depends"];
      for (auto d = depends.Begin(); d != depends.End(); d++) {
	a.depends.insert(d -> GetString());
      }
    }

    if (i -> value.HasMember("ship upgrades")) {
      auto &upgrades = i -> value["ship upgrades"];
      for (auto u = upgrades.MemberBegin(); u != upgrades.MemberEnd(); u++) {
	string ship_class = u -> name.GetString();
	for (auto v = u -> value.Begin(); v != u -> value.End(); v++) a.ship_upgrades[ship_class].insert(v -> GetString());
      }
    }

    tree[a.name] = a;
  }

  delete docp;
  init = true;
  return tree;
}

list<string> data::available() {
  list<string> res;

  for (auto &x : table()){
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
  ship_stats mod_stats = ship::table().at(s.ship_class);

  s.angle = utility::random_uniform(0, 2 * M_PI);
  
  // add upgrades from research tree
  auto &rtab = table();
  for (auto t : researched) {
    s.upgrades += rtab.at(t).ship_upgrades.at(s.class_id);
    s.upgrades += rtab.at(t).ship_upgrades.at(research::upgrade_all_ships);
  }

  // add upgrades from solar facilities
  for (auto &x : sol -> development) {
    facility_object t = x.second;
    s.upgrades += t.ship_upgrades.at(s.class_id);
    s.upgrades += t.ship_upgrades.at(research::upgrade_all_ships);
  }

  // evaluate upgrades
  auto &utab = upgrade::table();
  s.base_stats = mod_stats;
  for (auto u : s.upgrades) {
    mod_stats = utab.at(u).modify;
    s.base_stats += mod_stats;
  }

  // TODO: does assignment from parent class work?
  static_cast<ship_stats&>(s) = s.base_stats;
}

ship data::build_ship(string c, solar::ptr sol){
  ship s(ship::table().at(c));

  // apply id  
  static int idc = 0;
  s.id = identifier::make(ship::class_id, idc++);

  // apply upgrades
  repair_ship(s, sol);

  return s;
}

bool data::can_build_ship(string v, solar::ptr sol){
  if (!ship::table().count(v)) {
    throw runtime_error("Military template: no such ship class: " + v);
  }
  
  int facility = sol -> get_facility_level(keywords::key_military);
  ship s(ship::table().at(v));
  if (s.depends_facility_level > facility) return false;
  if (s.depends_tech.length() > 0 && !researched.count(s.depends_tech)) return false;
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
  x.allocation[keywords::key_development] = 1;
  x.allocation[keywords::key_mining] = 1;
  data["culture growth"] = x;

  // mining colony
  x = empty;
  x.allocation[keywords::key_culture] = 1;
  x.allocation[keywords::key_development] = 1;
  x.allocation[keywords::key_mining] = 3;
  data["mining colony"] = x;

  // military expansion
  x = empty;
  x.allocation[keywords::key_culture] = 1;
  x.allocation[keywords::key_development] = 1;
  x.allocation[keywords::key_military] = 3;
  x.allocation[keywords::key_mining] = 3;

  if (can_build_ship("bomber", sol)) x.military["bomber"] = 1;
  if (can_build_ship("fighter", sol)) x.military["fighter"] = 2;

  data["military expansion"] = x;

  // research & development
  x = empty;
  x.allocation[keywords::key_culture] = 1;
  x.allocation[keywords::key_development] = 1;
  x.allocation[keywords::key_research] = 1;
  x.allocation[keywords::key_mining] = 1;
  data["research"] = x;
  
  return data;
}
