#ifndef _STK_COMMAND
#define _STK_COMMAND

#include <set>
#include <list>
#include <string>

#include "types.h"

namespace st3{
  /*! struct representing a command */
  struct command{
    static const std::string action_waypoint;
    static const std::string action_follow;
    static const std::string action_join;
    static const std::string action_attack;
    static const std::string action_land;
    static const std::string action_colonize;

    combid source; /*!< key of entity holding the command */
    combid target; /*!< key of target */
    std::string action;
    std::set<combid> ships; /*!< ids of ships allocated to the command */

    /*! default constructor */
    command();
  };

  /*! check if two commands have the same source and target 
    @param a first command
    @param b second command
    @return a and b have the same source and the same target
  */
  bool operator == (const command &a, const command &b);
};

#endif
