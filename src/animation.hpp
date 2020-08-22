#pragma once

#include "animation_data.hpp"
#include "types.hpp"

namespace st3 {
struct animation : public animation_data {
  static constexpr float tmax = 4;
  float time;
  int frame0;

  animation(const animation_data &d);
};
};  // namespace st3
