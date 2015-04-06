#ifndef _STK_FLEET
#define _STK_FLEET

#include <set>
#include "types.h"

namespace st3{
  struct fleet{
    static idtype id_counter;
    std::set<idtype> ships;
    target_t target;
    point position;
    sint owner;
  };
};
#endif
