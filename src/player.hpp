#pragma once

#include <string>

#include "animation_data.hpp"
#include "types.hpp"
#include "upgrades.hpp"

namespace st3 {
/*! player data */
struct player {
  std::string name;              /*!< name of the player */
  sint color;                    /*!< the player's color */
  std::list<animation_data> animations;
  std::list<std::string> log;
  hm_t<std::string, upgrade> upgrades;
};
};  // namespace st3
