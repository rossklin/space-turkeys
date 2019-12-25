#ifndef _STK_ANIMATION
#define _STK_ANIMATION

#include "animation_data.h"
#include "types.h"

namespace st3 {
struct animation : public animation_data {
  static constexpr float tmax = 4;
  int frame;

  animation(const animation_data &d);
  float time_passed();
};
};  // namespace st3

#endif
