#pragma once

#include "types.hpp"

namespace st3 {
namespace cost {

struct allocation {
  hm_t<std::string, sfloat> data;

  void setup(std::vector<std::string> x);
  void confirm_content(std::vector<std::string> x);
  sfloat count();
  void normalize();
  void add(allocation a);
  void scale(float a);

  sfloat& operator[](const std::string& k);
};

struct ship_allocation : public allocation {
  ship_allocation();
};

float expansion_multiplier(float level);
};  // namespace cost
};  // namespace st3
