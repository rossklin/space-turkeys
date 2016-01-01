#include <cmath>
#include <iostream>

#include "graphics.h"
#include "types.h"

using namespace std;
using namespace st3;
using namespace graphics;

sf::Font graphics::default_font;

sf::Color graphics::sfcolor(sint c){
  sint mask = 0xff;
  return sf::Color(mask & (c >> 16), mask & (c >> 8), mask & c, mask & (c >> 24));
}

sf::Color graphics::fade_color(sf::Color from, sf::Color to, float r){
  return sf::Color(from.r + r * (to.r - from.r), 
		   from.g + r * (to.g - from.g), 
		   from.b + r * (to.b - from.b), 
		   from.a + r * (to.a - from.a));
}

void graphics::initialize(){
  
  // setup load text
  if (!default_font.loadFromFile("fonts/AjarSans-Regular.ttf")){
    cout << "error loading font" << endl;
    exit(-1);
  }
}

void graphics::draw_ship(window_t &w, ship s, sf::Color col, float sc){
  vector<sf::Vertex> svert;

  sf::Color cnose(255,200,180,200);

  switch (s.ship_class){
  case solar::ship_scout:
    svert.resize(4);
    svert[0] = sf::Vertex(point(2, 0), col);
    svert[1] = sf::Vertex(point(-2, -1), col);
    svert[2] = sf::Vertex(point(-2, 1), col);
    svert[3] = sf::Vertex(point(2, 0), col);
    break;
  case solar::ship_fighter:
    svert.resize(5);
    svert[0] = sf::Vertex(point(2, 0), cnose);
    svert[1] = sf::Vertex(point(-2, -1), col);
    svert[2] = sf::Vertex(point(-3, 0), col);
    svert[3] = sf::Vertex(point(-2, 1), col);
    svert[4] = sf::Vertex(point(2, 0), cnose);
    break;
  case solar::ship_bomber:
    svert.resize(7);
    svert[0] = sf::Vertex(point(2, 0), col);
    svert[1] = sf::Vertex(point(0, -3), cnose);
    svert[2] = sf::Vertex(point(-2, -3), cnose);
    svert[3] = sf::Vertex(point(-1, 0), col);
    svert[4] = sf::Vertex(point(-2, 3), cnose);
    svert[5] = sf::Vertex(point(0, 3), cnose);
    svert[6] = sf::Vertex(point(2, 0), col);
    break;
  case solar::ship_colonizer:
    svert.resize(5);
    svert[0] = sf::Vertex(point(2, 1), col);
    svert[1] = sf::Vertex(point(2, -1), col);
    svert[2] = sf::Vertex(point(-2, -1), col);
    svert[3] = sf::Vertex(point(-2, 1), col);
    svert[4] = sf::Vertex(point(2, 1), col);
    break;
  default:
    cout << "invalid ship type: " << s.ship_class << endl;
    exit(-1);
  }

  sf::Transform t;
  t.translate(s.position.x, s.position.y);
  t.rotate(s.angle / (2 * M_PI) * 360);
  t.scale(sc, sc);
  w.draw(&svert[0], svert.size(), sf::LinesStrip, t);
}

/* **************************************** */
/* INTERFACES */
/* **************************************** */

using namespace sfg;
using namespace interface;

// build and wrap in shared ptr

main_interface::Create(choice::choice c){return Ptr(new main_interface(c));}
research_window::Create(choice::c_research c){return Ptr(new research_window(c));}

solar_query::main_window::Create(solar::choice::choice_t c){return Ptr(new solar_query::main_window(c));}

solar_query::expansion::Create(solar::choice::c_expansion c){return Ptr(new solar_query::military(c));}
solar_query::military::Create(solar::choice::c_military c){return Ptr(new solar_query::military(c));}
solar_query::mining::Create(solar::choice::c_mining c){return Ptr(new solar_query::mining(c));}

// constructors

// main interface
main_interface::main_interface(choice::choice c) : response(c) {

}

// research window
research_window::research_window(choice::c_research c) : response(c) {

}

// main window for solar choice
solar_query::main_window::main_window(solar::choice::choice_t c) : response(c){
  int max_allocation;
  auto layout = Box::Create(Box::Orientation::VERTICAL);
  auto l_help = Label::Create("Click left/right to add/reduce");

  // sector allocation buttons
  auto b_culture = Button::Create("CULTURE");

  b_culture -> GetSignal(Widget::OnLeftClick).Connect([&response, max_allocation](){
      if (response.count_allocation() < max_allocation) response.allocation.culture++;
    };);

  b_culture -> GetSignal(Widget::OnRightClick).Connect([&response](){
      if (response.allocation.culture > 0) response.allocation.culture--;
    };);
  
  auto b_military = Button::Create("MILITARY");
  auto b_mining = Button::Create("MINING");
  auto b_expansion = Button::Create("EXANSION");
  auto b_accept = Button::Create("ACCEPT");
  auto b_cancel = Button::Create("CANCEL");

  // add sub window buttons

  // pack them in layout

  // add layout to this
}

// sub window for expansion choice
solar_query::expansion::expansion(solar::choice::c_expansion c) : response(c){

};

// sub window for military choice
solar_query::military::military(solar::choice::c_military c) : response(c){

};

// sub window for mining choice
solar_query::mining::s_mining(solar::choice::c_mining c) : response(c){

};
