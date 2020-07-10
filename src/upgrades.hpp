#pragma once

#include <functional>
#include <set>
#include <string>

#include "ship_stats.hpp"

namespace st3 {
class game_data;

class upgrade {
 public:
  static const hm_t<std::string, upgrade> &table();

  std::set<std::string> inter;
  hm_t<std::string, std::set<std::string> > hook;
  ssmod_t modify;
};
};  // namespace st3
