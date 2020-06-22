#pragma once

#include <rapidjson/document.h>

#include <set>
#include <string>

#include "cost.h"
#include "types.h"

namespace st3 {
namespace development {
class node {
 public:
  // boosts
  hm_t<std::string, sfloat> solar_modifier;
  hm_t<std::string, std::set<std::string> > ship_upgrades;

  // requirements
  sfloat cost_time;
  hm_t<std::string, sint> depends_facilities;
  std::set<std::string> depends_techs;

  // trackers
  std::string name;
  sfloat progress;
  sint level;

  node();
  bool parse(std::string name, const rapidjson::Value &v);
};

template <typename T>
hm_t<std::string, T> read_from_json(const rapidjson::Value &v, T init);
};  // namespace development
};  // namespace st3
