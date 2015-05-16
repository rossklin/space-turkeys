#ifndef _STK_WAYPOINT
#define _STK_WAYPOINT

#include <list>

#include "types.h"
#include "command.h"

namespace st3{
  /*! Waypoints allow position based fleet joining and splitting.*/
  struct waypoint{
    /*! ID counter for waypoints*/
    static idtype id_counter;

    /*! List of commands waiting to trigger when all ships have arrived */
    std::list<command> pending_commands;

    /*! Position of the waypoint */
    point position;
  };
};
#endif
