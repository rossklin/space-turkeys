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
}

const hm_t<string, tech>& data::table(){
  static hm_t<std::string, tech> tree;
  static bool init = false;

  if (init) return tree;

  tech t_init;
  // todo: set cost and facility level

  rapidjson::Document *docp = utility::get_json("research");
  tree = development::read_from_json<tech>((*docp)["research"], t_init);
  delete docp;
  init = true;
  return tree;
}

list<string> data::list_tech_requirements(string v) {
  list<string> req;
  tech t = table().at(v);
  if (researched.count(v)) {
    req.push_back("already researched");
  }else{
    if (t.cost_time > accumulated) req.push_back("research points");
    for (auto d : t.depends_techs) if (!researched.count(d)) req.push_back("technology " + d);
    for (auto f : t.depends_facilities) if (f.second > facility_level[f.first]) req.push_back(f.first + " level " + to_string(f.second));
  }
  return req;
}

list<string> data::available() {
  list<string> res;
  for (auto &x : table()) if (list_tech_requirements(x.first).empty()) res.push_back(x.first);
  return res;
}

void data::repair_ship(ship &s, solar::ptr sol) {
  ship_stats mod_stats = ship::table().at(s.ship_class);

  s.angle = utility::random_uniform(0, 2 * M_PI);

  auto maybe_asu = [](const development::node &n, string sc) -> set<string> {
    set<string> sum;
    if (n.ship_upgrades.count(sc)) sum += n.ship_upgrades.at(sc);
    if (n.ship_upgrades.count(research::upgrade_all_ships)) sum += n.ship_upgrades.at(research::upgrade_all_ships);
    return sum;
  };
  
  // add upgrades from research and facilities
  auto &rtab = table();
  for (auto t : researched) s.upgrades += maybe_asu(rtab.at(t), s.ship_class);
  for (auto &x : sol -> development) s.upgrades += maybe_asu(x.second, s.ship_class);

  // evaluate upgrades
  auto &utab = upgrade::table();
  s.base_stats = mod_stats;
  for (auto u : s.upgrades) {
    mod_stats = utab.at(u).modify;
    s.base_stats += mod_stats;
  }

  s.set_stats(s.base_stats);
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

bool data::can_build_ship(string v, solar::ptr sol, list<string> *data){
  if (!ship::table().count(v)) {
    throw runtime_error("Military template: no such ship class: " + v);
  }
  
  int facility = sol -> get_facility_level("shipyard");
  ship_stats s = ship::table().at(v);
  bool success = true;

  if (data) data -> clear();
  
  if (s.depends_facility_level > facility) {
    success = false;
    if (data) data -> push_back("shipyard level " + to_string(s.depends_facility_level));
  }
  
  if (s.depends_tech.length() > 0 && !researched.count(s.depends_tech)) {
    success = false;
    if (data) data -> push_back("research " + s.depends_tech);
  }
  
  return success;
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
