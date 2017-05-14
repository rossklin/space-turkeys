#include <string>

#include "types.h"
#include "development_tree.h"
#include "research.h"
#include "solar.h"

using namespace std;
using namespace st3;

void development::node::read_from_json(rapidjson::GenericValue &v) {
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
      for (auto v = u -> value.Begin(); v != u -> value.End(); v++) ship_upgrades[ship_name].insert(v -> GetString());
    }
  }

  if (v.HasMember("cost")) {
    auto &c = v["cost"];
    for (auto k : keywords::resource) {
      if (c.HasMember(k.c_str())) cost.res[k] = c[k.c_str()].GetDouble();
    }
    if (c.HasMember("time")) cost.time = c["time"].GetDouble();
  }

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
hm_t<string, T> development::read_from_json(rapidjson::GenericValue &doc, T base){
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

template<> hm_t<string, research::tech> read_from_json(const rapidjson::GenericValue &v);
template<> hm_t<string, facility> read_from_json(const rapidjson::GenericValue &v);
