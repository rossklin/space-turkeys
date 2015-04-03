#ifndef _STK_GRAPHICS
#define _STK_GRAPHICS

#include <string>
#include <SFML/Graphics.hpp>

#include "game_data.h"
#include "choice.h"

namespace st3{
  typedef sf::RenderWindow window_t;
  const std::string font_dir = "/usr/share/fonts/truetype/msttcorefonts/";

  void draw_universe(window_t &w, game_data &g);
  void draw_universe(window_t &w, game_data &g, choice &c);

  sf::Color sfcolor(sint);
};
#endif
