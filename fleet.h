#ifndef _STK_FLEET
#define _STK_FLEET

#include <set>
#include "types.h"
#include "command.h"

namespace st3{
  struct fleet{
    static idtype id_counter;
    static const int update_period = 10;
    static const int interact_d2 = 100;

    // serialized components
    std::set<idtype> ships;
    command com;
    point position;
    sfloat radius;
    sint owner;
    sbool converge;

    // mechanical components
    int update_counter;

    fleet();
  };
};
#endif
