#ifndef _STK_ANIMATION
#define _STK_ANIMATION

#include <chrono>
#include "types.h"
#include "animation_data.h"

namespace st3 {  
  struct animation {
    animation_data data;
    std::chrono::system_clock::time_point created;

    animation(const animation_data &d);
    float time_passed();
  };
};

#endif
