#ifndef _STK_SHIP
#define _STK_SHIP

#include "types.h"

namespace st3{

  class fleet;
  class data_grid;

  struct ship{
    sint id;
    point position;
    sfloat angle;
    sfloat speed;
    sint owner;
    sint hp;
    sbool was_killed;

    fleet *my_fleet;
    data_grid *grid;
  };

};
#endif
