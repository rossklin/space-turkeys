#ifndef _STK_COMMAND
#define _STK_COMMAND

#include <list>

#include "types.h"

namespace st3{
  struct command{
    // game data
    source_t source;
    target_t target;
    std::list<idtype> ships;

    std::list<command> child_commands;
    command();
  };

  bool operator == (const command &a, const command &b);
};

#endif
