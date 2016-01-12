#ifndef _STK_PLAYER
#define _STK_PLAYER

#include <string>
#include "types.h"
#include "research.h"

namespace st3{
  /*! player data */
  struct player{
    std::string name; /*!< name of the player */
    sint color; /*!< the player's color */
    research::data research_level; /*!< the player's research level */
  };
};
#endif
