#pragma once

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
