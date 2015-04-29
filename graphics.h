#ifndef _STK_GRAPHICS
#define _STK_GRAPHICS

#include <string>
#include <SFML/Graphics.hpp>

#include "game_data.h"
#include "choice.h"
#include "types.h"
#include "ship.h"

namespace st3{
  typedef sf::RenderWindow window_t;
  namespace graphics{

    extern sf::Font default_font;

    // default colors
    const sf::Color solar_neutral(150,150,150);
    const sf::Color command_selected_head(255,255,255,255);
    const sf::Color command_selected_body(150,240,150,230);
    const sf::Color command_selected_tail(80,200,80,130);
    const sf::Color command_selected_text(200,200,200);
    const sf::Color command_normal_head(255,255,255,200);
    const sf::Color command_normal_body(100,200,100,200);
    const sf::Color command_normal_tail(50,100,50,100);
    const sf::Color command_normal_text(200,200,200,100);
    const sf::Color solar_fill(10,20,30,40);
    const sf::Color solar_selected(255,255,255,180);
    const sf::Color fleet_fill(200,200,200,50);
    const sf::Color fleet_outline(40,60,180,200);

    sf::Color sfcolor(sint);

    void initialize();
    void draw_ship(sf::RenderWindow &w, ship s, sf::Color c, float sc = 1);
  };
};
#endif
