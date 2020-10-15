#pragma once

#include <vector>

#include "game_base_data.hpp"
#include "selector.hpp"
#include "types.hpp"

namespace st3 {
// Class that precaches some data for loading into main client::game
class client_data_frame : public game_base_data {
 public:
  std::vector<point> enemy_clusters;
  std::vector<specific_selector<fleet>::ptr> fleets;
  std::vector<specific_selector<waypoint>::ptr> waypoints;
  std::vector<idtype> new_tids;
};
};  // namespace st3