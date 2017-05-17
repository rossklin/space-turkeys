#ifndef _STK_COMMAND
#define _STK_COMMAND

#include <set>
#include <list>
#include <string>

#include "types.h"

namespace st3{
  /*! struct representing a command */
  struct command{
    combid source; /*!< key of entity holding the command */
    combid origin; /*!< key of the solar where the command was generated from */
    combid target; /*!< key of target */
    std::string action;
    sint policy;
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
