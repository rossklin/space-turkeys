#include <cmath>

#include "graphics.h"
#include "types.h"

using namespace std;
using namespace st3;

const sf::Vertex ship_pts[] = {
  sf::Vertex(sf::Vector2f(10, 0)),
  sf::Vertex(sf::Vector2f(-10, -5)),
  sf::Vertex(sf::Vector2f(-10, 5)),
  sf::Vertex(sf::Vector2f(10, 0))
};

void draw_ship(window_t &w, const ship &s){
  sf::Transform t;
  t.translate(s.position.x, s.position.y);
  t.rotate(s.angle / (2 * M_PI) * 360);

  sf::RenderStates states(t);

  w.draw(ship_pts, 4, sf::LinesStrip, states);
}

void st3::draw_universe(window_t &w, game_data &g){
  for (auto s : g.ships) draw_ship(w, s.second);
}

void st3::draw_universe(window_t &w, game_data &g, choice &c){
  draw_universe(w, g);

  // draw choice stuff
}
