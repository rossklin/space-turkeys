#ifndef _STK_PROTOCOL
#define _STK_PROTOCOL

#include "types.h"

namespace st3{
  namespace protocol{
    // queries
    const sint game_round = 0; 
    const sint choice = 1;
    const sint frame = 2;

    // responses
    const sint confirm = 3;
    const sint standby = 4;
    const sint complete = 5;
    const sint invalid = 6;
  };
};
#endif
