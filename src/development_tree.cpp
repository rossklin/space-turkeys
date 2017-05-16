#include <string>
#include <rapidjson/document.h>
#include <boost/algorithm/string.hpp>

#include "types.h"
#include "development_tree.h"
#include "research.h"
#include "solar.h"
#include "ship.h"

using namespace std;
using namespace st3;

development::node::node() {
  cost_time = 0;
}

void development::node::read_from_json(const rapidjson::Value &v) {
  auto &stab = ship::table();
  
  if (v.HasMember("sector boost")) {
    auto &secboost = v["sector boost"];
    for (auto k : keywords::sector) {
      if (secboost.HasMember(k.c_str())) sector_boost[k] = secboost[k.c_str()].GetDouble();
    }
  }

  if (v.HasMember("ship upgrades")) {
    auto &upgrades = v["ship upgrades"];
    for (auto u = upgrades.MemberBegin(); u != upgrades.MemberEnd(); u++) {
      string ship_name = u -> name.GetString();
      list<string> ship_classes;
      if (ship_name == research::upgrade_all_ships) {
	ship_classes = ship::all_classes();
      } else if (ship_name[0] == '#' || ship_name[0] == '!') {
	// list of tag conditions for set of upgrades
	vector<string> conditions;
	boost::split(conditions, ship_name, [](char c) {return c == ':';});
	
	for (auto &s : stab) {
	  for (auto cond : conditions) {
	    if (cond[0] == '#' && !s.second.tags.count(cond.substr(1))) continue;
	    if (cond[0] == '!' && s.second.tags.count(cond.substr(1))) continue;
	    ship_classes.push_back(s.first);
	  }
	}
      } else {
	ship_classes.push_back(ship_name);
      }
      
      for (auto sc : ship_classes) {
	for (auto v = u -> value.Begin(); v != u -> value.End(); v++) {
	  ship_upgrades[sc].insert(v -> GetString());
	}
      }
    }
  }
  
  if (v.HasMember("build time")) cost_time = v["build time"].GetDouble();

  if (v.HasMember("depends facilities")) {
    auto &dep = v["depends facilities"];
    for (auto d = dep.MemberBegin(); d != dep.MemberEnd(); d++) {
      depends_facilities[d -> name.GetString()] = d -> value.GetInt();
    }
  }

  if (v.HasMember("depends technologies")) {
    auto &dep = v["depends technologies"];
    for (auto d = dep.Begin(); d != dep.End(); d++) depends_techs.insert(d -> GetString());
  }
}

template<typename T>
hm_t<string, T> development::read_from_json(const rapidjson::Value &doc, T base){
  static_assert(is_base_of<development::node, T>::value, "Read development::node");
  
  hm_t<string, T> data;
  T a;
  
  for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); i++){
    a = base;
    a.name = i -> name.GetString();
    a.read_from_json(i -> value);
    data[a.name] = a;
  }

  return data;
}

template hm_t<string, research::tech> st3::development::read_from_json<research::tech>(const rapidjson::Value &v, research::tech b);
template hm_t<string, facility> st3::development::read_from_json<facility>(const rapidjson::Value &v, facility b);
