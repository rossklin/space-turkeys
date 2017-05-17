#ifndef _STK_EXPLOSION
#define _STK_EXPLOSION

#include <chrono>
#include <SFML/Graphics.hpp>
#include "types.h"

namespace st3 {
  struct explosion {
    std::chrono::system_clock::time_point created;
    point position;
    sf::Color color;

    explosion(point p, sint c);
    float time_passed();
  };
};

#endif
