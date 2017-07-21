#ifndef _STK_ANIMATION_DATA
#define _STK_ANIMATION_DATA

#include "types.h"

namespace st3 {
  struct animation_tracker_info {
    point p;
    point v;
    combid eid;
  };
  
  struct animation_data {
    enum category {
      explosion, shot, shield, bomb
    };

    animation_tracker_info t1;
    animation_tracker_info t2;
    sint delay;
    sfloat magnitude;
    sfloat radius;
    sint color;
    sint cat;
  };
};

#endif
