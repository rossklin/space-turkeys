#ifndef _STK_SHIP
#define _STK_SHIP

#include <string>
#include <vector>

#include "types.h"

namespace st3{
  struct research;

  struct ship{
    static idtype id_counter;
    typedef sint class_t;

    // serialised variables
    idtype fleet_id;
    class_t ship_class;
    point position;
    sfloat angle;
    sfloat speed;
    sint owner;
    sint hp;
    sfloat interaction_radius;

    // buffer data
    bool was_killed;

    ship();
    ship(class_t c, research &r);
  };
};
#endif
