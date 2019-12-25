#ifndef _STK_EXPLOSION
#define _STK_EXPLOSION

#include <SFML/Graphics.hpp>
#include <chrono>
#include "types.h"

namespace st3 {
struct explosion {
  std::chrono::system_clock::time_point created;
  point position;
  sf::Color color;

  explosion(point p, sint c);
  float time_passed();
};
};  // namespace st3

#endif
