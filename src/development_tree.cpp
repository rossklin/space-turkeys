#include <string>
#include <rapidjson/document.h>
#include <boost/algorithm/string.hpp>

#include "types.h"
#include "development_tree.h"
#include "research.h"
#include "solar.h"
#include "ship.h"
#include "utility.h"

using namespace std;
using namespace st3;

development::node::node() {
  level = 0;
  cost_time = 0;
  progress = 0;
}

bool development::node::parse(string name, const rapidjson::Value &v) {
  auto &stab = ship::table();
    
  if (name == "solar modifier") {
    for (auto u = v.MemberBegin(); u != v.MemberEnd(); u++) {
      string sec_name = u -> name.GetString();
      float sec_value = u -> value.GetDouble();
      if (utility::find_in(sec_name, keywords::solar_modifier)) {
	solar_modifier[sec_name] = sec_value;
      } else {
	return false;
      }
    }
  } else if (name == "ship upgrades") {
    for (auto u = v.MemberBegin(); u != v.MemberEnd(); u++) {
      string ship_name = u -> name.GetString();
      list<string> ship_classes;
	
      if (ship_name == research::upgrade_all_ships) {
	ship_classes = ship::all_classes();
      } else if (ship_name[0] == '[' || ship_name[0] == '#' || ship_name[0] == '!') {
	// list of tag conditions for set of upgrades
	vector<string> conditions;
	boost::split(conditions, ship_name, [](char c) {return c == ':';});
	
	for (auto &s : stab) {
	  bool pass = true;
	  for (auto cond : conditions) {
	    if (cond[0] == '#') {
	      if (!s.second.tags.count(cond.substr(1))) pass = false;
	    } else if (cond[0] == '!'){
	      if (s.second.tags.count(cond.substr(1))) pass = false;
	    } else if (cond[0] == '[') {
	      vector<string> limits;
	      string range = cond.substr(1, cond.length() - 2);
	      boost::split(limits, range, [](char c){return c == ',';});
	      float lower = stof(limits[0]);
	      float upper = stof(limits[1]);
	      if (s.second.stats[sskey::key::mass] < lower || s.second.stats[sskey::key::mass] > upper) pass = false;
	    } else {
	      throw parse_error("invalid condition: " + cond);
	    }
	  }
	  if (pass) ship_classes.push_back(s.first);
	}
      } else if (utility::find_in(ship_name, ship::all_classes())) {
	ship_classes.push_back(ship_name);
      } else {
	throw parse_error("invalid upgrade ship class specifier: " + ship_name);
      }
      
      for (auto sc : ship_classes) {
	for (auto x = u -> value.Begin(); x != u -> value.End(); x++) {
	  ship_upgrades[sc].insert(x -> GetString());
	}
      }
    }
  } else if (name == "build time") {
    cost_time = v.GetDouble();
  } else if (name == "depends facilities") {
    for (auto d = v.MemberBegin(); d != v.MemberEnd(); d++) {
      depends_facilities[d -> name.GetString()] = d -> value.GetInt();
    }
  } else if (name == "depends technologies") {
    for (auto d = v.Begin(); d != v.End(); d++) {
      depends_techs.insert(d -> GetString());
    }
  } else {
    return false;
  }

  return true;
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
