#ifndef _STK_GRAPHICS
#define _STK_GRAPHICS

#include <SFML/Graphics.hpp>

#include "game_data.h"
#include "choice.h"

namespace st3{
  typedef sf::RenderWindow window_t;

  void draw_universe(window_t &w, game_data &g);
  void draw_universe(window_t &w, game_data &g, choice &c);
};
#endif
