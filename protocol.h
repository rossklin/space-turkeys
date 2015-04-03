#ifndef _STK_PROTOCOL
#define _STK_PROTOCOL

#include "types.h"

namespace st3{
  namespace protocol{
    // queries
    const sint game_round = 0; 
    const sint choice = 1;
    const sint frame = 2;
    const sint connect = 3;

    // responses
    const sint confirm = 1004;
    const sint standby = 1005;
    const sint complete = 1006;
    const sint invalid = 1007;
  };
};
#endif
