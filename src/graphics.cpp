#include <cmath>
#include <iostream>
#include <string>

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

void graphics::draw_circle(window_t &w, point p, float r, sf::Color co, sf::Color cf, float b) {
  sf::CircleShape sol(r);
  float inv = inverse_scale(w).x;
  sol.setPointCount(r / inv);
  sol.setFillColor(cf);
  sol.setOutlineThickness(b);
  sol.setOutlineColor(co);
  sol.setPosition(p.x - r, p.y - r);
  w.draw(sol);
};

void graphics::draw_text(window_t &w, string v, point p, int fs, bool ul) {
  point inv = graphics::inverse_scale(w);
  sf::Text text;
  text.setString(v);
  text.setFont(graphics::default_font); 
  text.setCharacterSize(fs);
  sf::FloatRect text_dims = text.getLocalBounds();
  if (ul) {
    text.setOrigin(point(text_dims.left, text_dims.top));
  }else{
    text.setOrigin(point(text_dims.left + text_dims.width/2, text_dims.top + text_dims.height / 2));
  }
  text.setPosition(p); 
  text.setColor(sf::Color(200,200,200));
  text.setScale(inv);
  w.draw(text);
};

void graphics::draw_framed_text(window_t &w, string v, sf::FloatRect bounds, sf::Color co, sf::Color cf) {
  int margin = ceil(bounds.height / 10);
  sf::RectangleShape r = graphics::build_rect(bounds);
  r.setOutlineColor(co);
  r.setFillColor(cf);
  r.setOutlineThickness(-margin);
  w.draw(r);

  int fs = bounds.height - 2 * margin;
  point p(bounds.left + bounds.width / 2, bounds.top + bounds.height / 2);
  draw_text(w, v, p, fs);
}

sfg::Button::Ptr graphics::ship_button(string ship_class, float width, float height, sf::Color col) {
  sf::RenderTexture tex;
  if (!tex.create(width, height)) {
    throw runtime_error("Failed to create render texture!");
  }

  ship s(ship::table().at(ship_class));
  s.position = point(width/2, height/2);
  float scale = width / 6;
  draw_ship(tex, s, col, scale);
  tex.display();

  sfg::Button::Ptr b = sfg::Button::Create();
  b -> SetImage(tex.getTexture().copyToImage());
  return b;
};


void graphics::draw_ship(sf::RenderTarget &w, ship s, sf::Color col, float sc){
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
