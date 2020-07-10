#pragma once

#include <SFML/Graphics.hpp>
#include <chrono>

#include "types.hpp"

namespace st3 {
struct explosion {
  std::chrono::system_clock::time_point created;
  point position;
  sf::Color color;

  explosion(point p, sint c);
  float time_passed();
};
};  // namespace st3
