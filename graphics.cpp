#include <cmath>
#include <iostream>

#include "graphics.h"
#include "types.h"

using namespace std;
using namespace st3;

sf::Font graphics::default_font;

sf::Color st3::graphics::sfcolor(sint c){
  sint mask = 0xff;
  return sf::Color(mask & (c >> 16), mask & (c >> 8), mask & c, mask & (c >> 24));
}

void st3::graphics::initialize(){
  
  // setup load text
  if (!graphics::default_font.loadFromFile("fonts/AjarSans-Regular.ttf")){
    cout << "error loading font" << endl;
    exit(-1);
  }
}

void st3::graphics::draw_ship(sf::RenderWindow &w, ship s, sf::Color col){
  sf::Vertex svert[4];
  svert[0] = sf::Vertex(point(10, 0), col);
  svert[1] = sf::Vertex(point(-10, -5), col);
  svert[2] = sf::Vertex(point(-10, 5), col);
  svert[3] = sf::Vertex(point(10, 0), col);
      
  sf::Transform t;
  t.translate(s.position.x, s.position.y);
  t.rotate(s.angle / (2 * M_PI) * 360);
  w.draw(svert, 4, sf::LinesStrip, t);
}
