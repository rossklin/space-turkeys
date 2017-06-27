#ifndef _STK_ANIMATION_DATA
#define _STK_ANIMATION_DATA

#include "types.h"

namespace st3 {
  struct animation_data {
    enum category {
      explosion, shot, shield, bomb
    };
    
    point p1;
    point p2;
    point v1;
    point v2;
    sfloat magnitude;
    sfloat radius;
    sint color;
    sint cat;
  };
};

#endif
