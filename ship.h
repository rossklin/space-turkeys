#ifndef _STK_SHIP
#define _STK_SHIP

#include "types.h"

namespace st3{
  struct ship{
    static idtype id_counter;

    point position;
    sfloat angle;
    sfloat speed;
    sint owner;
    sint hp;
    sbool was_killed;
  };

};
#endif
