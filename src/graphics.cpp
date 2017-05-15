#include <cmath>
#include <iostream>
#include <boost/algorithm/string/join.hpp>

#include "graphics.h"
#include "types.h"
#include "utility.h"
#include "cost.h"
#include "client_game.h"
#include "explosion.h"

using namespace std;
using namespace st3;
using namespace graphics;

sf::Font graphics::default_font;

sf::Color graphics::fade_color(sf::Color from, sf::Color to, float r){
  return sf::Color(from.r + r * (to.r - from.r), 
		   from.g + r * (to.g - from.g), 
		   from.b + r * (to.b - from.b), 
		   from.a + r * (to.a - from.a));
}

void graphics::initialize(){
  
  // setup load text
  if (!default_font.loadFromFile("fonts/AjarSans-Regular.ttf")){
    throw runtime_error("error loading font");
  }
}

void graphics::draw_ship(window_t &w, ship s, sf::Color col, float sc){
  vector<sf::Vertex> svert;

  sf::Color cnose(255,200,180,200);

  svert.resize(s.shape.size());
  for (int i = 0; i < s.shape.size(); i++) {
    svert[i].position = s.shape[i].first;
    svert[i].color = s.shape[i].second == 'p' ? col : cnose;
  }

  sf::Transform t;
  t.translate(s.position.x, s.position.y);
  t.rotate(s.angle / (2 * M_PI) * 360);
  t.scale(sc, sc);
  w.draw(&svert[0], svert.size(), sf::LinesStrip, t);
}


// sfml stuff
// make an sf::RectangleShape with given bounds
sf::RectangleShape graphics::build_rect(sf::FloatRect bounds){
  sf::RectangleShape r;
  r.setSize(sf::Vector2f(bounds.width, bounds.height));
  r.setPosition(sf::Vector2f(bounds.left, bounds.top));
  return r;
}

// point coordinates of view ul corner
point graphics::ul_corner(window_t &w){
  sf::View sv = w.getView();
  point v = sv.getSize();
  return sv.getCenter() - point(v.x * 0.5, v.y * 0.5);
}

// transform from pixels to points
sf::Transform graphics::view_inverse_transform(window_t &w){
  sf::Transform t;
  sf::View v = w.getView();

  t.translate(ul_corner(w));
  t.scale(v.getSize().x / w.getSize().x, v.getSize().y / w.getSize().y);
  return t;
}

// scale from pixels to points
point graphics::inverse_scale(window_t &w){
  sf::View v = w.getView();
  return point(v.getSize().x / w.getSize().x, v.getSize().y / w.getSize().y);
}

/* **************************************** */
/* INTERFACES */
/* **************************************** */

using namespace sfg;
using namespace interface;

// build and wrap in shared ptr

bottom_panel::Ptr bottom_panel::Create(bool &done, bool &accept){
  auto buf = Ptr(new bottom_panel());
  
  auto layout = Box::Create(Box::Orientation::HORIZONTAL);
  auto b_proceed = Button::Create("PROCEED");

  b_proceed -> GetSignal(Widget::OnLeftClick).Connect([&done, &accept](){
      done = true;
      accept = true;
    });

  layout -> Pack(b_proceed);
  buf -> Add(layout);
  buf -> SetAllocation(sf::FloatRect(0.3 * desktop_dims.x, bottom_start, 0.4 * desktop_dims.x, desktop_dims.y - bottom_start));
  return buf;
}

// top_panel::Ptr top_panel::Create(){
//   auto buf = Ptr(new top_panel());

//   auto layout = Box::Create(Box::Orientation::HORIZONTAL);
//   auto b_research = Button::Create("RESEARCH");

//   b_research -> GetSignal(Widget::OnLeftClick).Connect([](){
//       desktop -> reset_qw(research_window::Create(&desktop -> response.research));
//     });

//   layout -> Pack(b_research);
//   buf -> Add(layout);
//   buf -> SetAllocation(sf::FloatRect(0, 0, desktop_dims.x, top_height));
//   return buf;
// }

// research_window::Ptr research_window::Create(choice::c_research *c){return Ptr(new research_window(c));}


void graphics::draw_explosion(window_t &w, explosion e){
  float t = e.time_passed();
  float rad = 20 * t * exp(-pow(3 * t,2));
  sf::Color c = fade_color(c, sf::Color::White, 0.5);
  c.a = 100;
  
  sf::CircleShape s(rad);
  s.setFillColor(c);
  s.setPosition(e.position - point(rad, rad));
  w.draw(s);
}
