#ifndef _STK_COMMAND
#define _STK_COMMAND

#include "types.h"

namespace st3{
  struct command{
    // game data
    source_t source;
    target_t target;
    sint quantity;

    // interface control variables
    sbool locked;
    sint lock_qtty;

    command();
  };
};

#endif
