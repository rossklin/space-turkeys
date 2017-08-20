#ifndef _STK_ANIMATION
#define _STK_ANIMATION

#include "types.h"
#include "animation_data.h"

namespace st3 {  
  struct animation : public animation_data {
    static constexpr float tmax = 4;
    int frame;

    animation(const animation_data &d);
    float time_passed();
  };
};

#endif
