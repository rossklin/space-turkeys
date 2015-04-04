#ifndef _STK_SOLAR
#define _STK_SOLAR

#include "types.h"

namespace st3{
  struct solar{
    static idtype id_counter;

    point position;
    sint owner;
    sfloat radius;
  };
};
#endif
