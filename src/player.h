#pragma once

#include <string>
#include "types.h"
#include "research.h"
#include "animation_data.h"
#include "cost.h"

namespace st3{
  /*! player data */
  struct player{
    std::string name; /*!< name of the player */
    sint color; /*!< the player's color */
    research::data research_level; /*!< the player's research level */
    std::list<animation_data> animations;
    std::list<std::string> log;
  };
};
