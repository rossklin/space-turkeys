#ifndef _STK_PLAYER
#define _STK_PLAYER

#include <string>
#include "types.h"
#include "research.h"

namespace st3{
  struct player{
    std::string name;
    sint color;
    research research_level;
  };
};
#endif
