#pragma once

#include <string>

#include "types.hpp"

namespace st3 {
struct animation_tracker_info {
  point p;
  point v;
  combid eid;
};

struct animation_data {
  enum category {
    explosion,
    shot,
    shield,
    bomb,
    message
  };

  animation_tracker_info t1;
  animation_tracker_info t2;
  sint delay;
  sfloat magnitude;
  sfloat radius;
  sint color;
  sint cat;
  std::string text;
};
};  // namespace st3
