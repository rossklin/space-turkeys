#ifndef _STK_WAYPOINT
#define _STK_WAYPOINT

#include <set>
#include <list>

#include "types.h"
#include "command.h"

namespace st3{
  struct waypoint{
    static idtype id_counter;
    std::list<command> pending_commands;
    point position;
  };
};
#endif
