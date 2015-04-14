#ifndef _STK_FLEET
#define _STK_FLEET

#include <list>
#include "types.h"
#include "command.h"

namespace st3{
  struct fleet{
    static idtype id_counter;
    std::list<idtype> ships;
    command com;
    point position;
    sfloat radius;
    sint owner;
  };
};
#endif
