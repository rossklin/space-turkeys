#include "fixed_star.h"
#include "utility.h"

using namespace st3::utility;

st3::client::fixed_star::fixed_star(point p) {
	float redshift;
	
	radius = 1;
	position = p;
	
	redshift = utility::random_uniform ();
	
	color.r = 255;
	color.g = 255 - 60 * redshift;
	color.b = 255 - 60 * redshift;
	color.a = 2 + 55 * utility::random_uniform ();
}

void st3::client::fixed_star::draw(st3::window_t &w) {
	sf::CircleShape star(radius);
	
	star.setRadius(radius * utility::inverse_scale(w).x);
	star.setPointCount(10);
	star.setFillColor(color);
	star.setPosition(position.x - radius, position.y - radius);
	w.draw(star);
}
