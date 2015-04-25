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
