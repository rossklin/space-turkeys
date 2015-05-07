#ifndef _STK_FLEET
#define _STK_FLEET

#include <set>
#include "types.h"
#include "command.h"

namespace st3{
  struct fleet{
    static idtype id_counter;
    static const int update_period = 1;
    static const int interact_d2 = 100;
    static constexpr float min_radius = 10;

    // serialized components
    std::set<idtype> ships;
    command com;
    point position;
    sfloat radius;
    sfloat vision;
    sint owner;
    sbool converge;

    // mechanical components
    int update_counter;
    float speed_limit;
    
    fleet();
    bool is_idle();
  };
};
#endif
