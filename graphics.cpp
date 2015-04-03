#include <cmath>
#include <iostream>

#include "graphics.h"
#include "types.h"

using namespace std;
using namespace st3;

sf::Color st3::sfcolor(sint c){
  sint mask = 0xff;
  return sf::Color(mask & (c >> 16), mask & (c >> 8), mask & c, mask & (c >> 24));
}


void st3::draw_universe(window_t &w, game_data &g){
  sf::Color col;
  sf::Vertex svert[4];

  // draw ships
  for (auto x : g.ships) {
    ship s = x.second;
    if (!s.was_killed){
      col = sfcolor(g.players[s.owner].color);
      svert[0] = sf::Vertex(sf::Vector2f(10, 0), col);
      svert[1] = sf::Vertex(sf::Vector2f(-10, -5), col);
      svert[2] = sf::Vertex(sf::Vector2f(-10, 5), col);
      svert[3] = sf::Vertex(sf::Vector2f(10, 0), col);
      
      sf::Transform t;
      t.translate(s.position.x, s.position.y);
      t.rotate(s.angle / (2 * M_PI) * 360);
      w.draw(svert, 4, sf::LinesStrip, t);
    }
  }

  // draw solars
  for (auto x : g.solars){
    solar s = x.second;
    sf::CircleShape sol(s.radius);
    sol.setFillColor(sf::Color(10,20,30,40));
    sol.setOutlineThickness(s.radius / 3);
    col = s.owner > -1 ? sfcolor(g.players[s.owner].color) : sf::Color(150,150,150);
    sol.setOutlineColor(col);
    sol.setPosition(s.position.x, s.position.y);
    w.draw(sol);
  }
}

void st3::draw_universe(window_t &w, game_data &g, choice &c){
  draw_universe(w, g);

  // draw choice stuff
}
