#include "fixed_star.h"
#include "utility.h"

using namespace st3::utility;

st3::client::fixed_star::fixed_star(point p) {
  radius = 1;
  position = p;
  float redshift = utility::random_uniform();

  color.r = 255;
  color.g = 255 - 30 * redshift;
  color.b = 255 - 30 * redshift;
  color.a = 20 + 55 * utility::random_uniform();
}

void st3::client::fixed_star::draw(st3::window_t &w) {
  sf::CircleShape star(radius);

  star.setRadius(radius * graphics::inverse_scale(w).x);
  star.setPointCount(10);
  star.setFillColor(color);
  star.setPosition(position.x - radius, position.y - radius);
  w.draw(star);
}

bool st3::client::fixed_star::operator==(const fixed_star &star) {
  return star.position == position;
}
