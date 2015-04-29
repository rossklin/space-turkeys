#ifndef _STK_COMMAND
#define _STK_COMMAND

#include <set>
#include <list>

#include "types.h"

namespace st3{
  struct command{
    // game data
    source_t source;
    target_t target;
    std::set<idtype> ships;

    command();
  };

  bool operator == (const command &a, const command &b);
};

#endif
