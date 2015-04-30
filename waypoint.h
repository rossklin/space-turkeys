#ifndef _STK_WAYPOINT
#define _STK_WAYPOINT

#include <set>
#include <list>

#include "types.h"
#include "command.h"

namespace st3{
  struct waypoint{
    static idtype id_counter;

    // serialized components
    std::list<command> pending_commands;
    point position;
    std::set<idtype> landed_ships;

    // buffer
    bool keep_me;
  };
};
#endif
