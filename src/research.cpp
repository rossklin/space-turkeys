#include "research.h"

#include <rapidjson/document.h>

#include <mutex>
#include <numeric>
#include <vector>

#include "choice.h"
#include "cost.h"
#include "fleet.h"
#include "ship.h"
#include "upgrades.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace research;
using namespace cost;

const int BASE_FLEETS = 4;
const int BASE_SHIPS_PER_FLEET = 9;

data::data() {}

const hm_t<string, tech> &data::table() {
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

set<string> data::get_tech_upgrades(string sc, string tech) {
  set<string> res;
  research::tech t = research::data::table().at(tech);
  res += t.ship_upgrades[sc];
  for (auto d : t.depends_techs) res += get_tech_upgrades(sc, d);
  return res;
};

list<string> data::list_tech_requirements(string v) const {
  list<string> req;
  tech t = table().at(v);
  if (researched().count(v)) {
    req.push_back("Already researched");
  } else {
    for (auto d : t.depends_techs)
      if (!researched().count(d)) req.push_back("technology " + d);
    for (auto f : t.depends_facilities) {
      int flev = 0;
      if (facility_level.count(f.first)) flev = facility_level.at(f.first);
      if (f.second > flev) req.push_back(f.first + " level " + to_string(f.second));
    }
  }
  return req;
}

list<string> data::available() const {
  list<string> res;
  for (auto &x : table())
    if (list_tech_requirements(x.first).empty()) res.push_back(x.first);
  return res;
}

void data::repair_ship(ship &s) const {
  ship_stats base_stats = ship::table().at(s.ship_class);

  auto maybe_asu = [](const development::node &n, string sc) -> set<string> {
    set<string> sum;
    if (n.ship_upgrades.count(sc)) sum += n.ship_upgrades.at(sc);
    if (n.ship_upgrades.count(research::upgrade_all_ships)) sum += n.ship_upgrades.at(research::upgrade_all_ships);
    return sum;
  };

  // add upgrades from research and facilities
  auto &rtab = table();
  for (auto t : researched()) s.upgrades += maybe_asu(rtab.at(t), s.ship_class);

  // evaluate upgrades
  auto &utab = upgrade::table();
  ssmod_t mod_stats;
  for (auto u : s.upgrades) mod_stats.combine(utab.at(u).modify);

  s.base_stats.stats = base_stats.stats;
  s.base_stats.modify_with(mod_stats);
  s.stats = s.base_stats.stats;
}

ship data::build_ship(idtype id, string c) const {
  ship s(ship::table().at(c));

  s.id = identifier::make(ship::class_id, id);

  // apply upgrades
  repair_ship(s);

  return s;
}

bool data::can_build_ship(string v, solar::ptr sol, list<string> *data) const {
  if (!ship::table().count(v)) {
    throw logical_error("Military template: no such ship class: " + v);
  }

  int facility = sol->effective_level(keywords::key_shipyard);
  ship_stats s = ship::table().at(v);
  bool success = true;

  if (data) data->clear();

  if (s.depends_facility_level > facility) {
    success = false;
    if (data) data->push_back("shipyard level " + to_string(s.depends_facility_level));
  }

  if (s.depends_tech.length() > 0 && !researched().count(s.depends_tech)) {
    success = false;
    if (data) data->push_back("research " + s.depends_tech);
  }

  return success;
}

set<string> data::researched() const {
  set<string> x;
  for (auto &t : tech_map)
    if (t.second.level > 0) x.insert(t.first);
  return x;
}

tech &data::access(string v) {
  auto &rtab = table();
  if (!rtab.count(v)) {
    throw logical_error("Attempted to access invalid tech: " + v);
  } else if (!tech_map.count(v)) {
    tech_map[v] = rtab.at(v);
  }

  return tech_map[v];
}

float data::solar_modifier(string k) const {
  float sum = 0;
  for (auto v : researched()) {
    tech x = tech_map.at(v);
    if (x.solar_modifier.count(k)) sum += x.solar_modifier[k];
  }
  return sum;
}

int data::get_max_fleets() const {
  int sum = BASE_FLEETS;
  for (auto v : researched()) sum += tech_map.at(v).increase_fleets;
  return sum;
}

int data::get_max_ships_per_fleet() const {
  int sum = BASE_SHIPS_PER_FLEET;
  for (auto v : researched()) sum += tech_map.at(v).increase_ships_per_fleet;
  return sum;
}

float data::get_order_modifier() const {
  float sum = 0;
  for (auto v : researched()) sum += tech_map.at(v).order_modifier;
  return sum;
}

void tech::read_from_json(const rapidjson::Value &x) {
  for (auto i = x.MemberBegin(); i != x.MemberEnd(); i++) {
    string name = i->name.GetString();
    if (!development::node::parse(name, i->value)) {
      if (name == "increase fleets") {
        increase_fleets = i->value.GetInt();
      } else if (name == "increase ships per fleet") {
        increase_ships_per_fleet = i->value.GetInt();
      } else if (name == "order modifier") {
        order_modifier = i->value.GetFloat();
      } else {
        throw parse_error("Failed to parse tech: " + name);
      }
    }
  }

  // sanity check
  if (cost_time <= 0) {
    throw parse_error("tech::read_from_json: invalid build time!");
  }
}

tech::tech() : development::node() {}
tech::tech(const tech &t) : development::node(t) {}
