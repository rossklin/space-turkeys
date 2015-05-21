#ifndef _STK_COMMAND
#define _STK_COMMAND

#include <set>
#include <list>

#include "types.h"

namespace st3{
  /*! struct representing a command */
  struct command{
    source_t source; /*!< key of entity holding the command */
    target_t target; /*!< key of target */
    std::set<idtype> ships; /*!< ids of ships allocated to the command */

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
