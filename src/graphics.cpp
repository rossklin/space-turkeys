#include <cmath>
#include <iostream>
#include <string>
#include <memory>

#include "graphics.h"
#include "types.h"
#include "utility.h"
#include "cost.h"
#include "client_game.h"
#include "animation.h"

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

void graphics::draw_circle(sf::RenderTarget &w, point p, float r, sf::Color co, sf::Color cf, float b) {
  sf::CircleShape sol(r);
  float inv = inverse_scale(w).x;
  sol.setPointCount(r / inv);
  sol.setFillColor(cf);
  sol.setOutlineThickness(b);
  sol.setOutlineColor(co);
  sol.setPosition(p.x - r, p.y - r);
  w.draw(sol);
};

void graphics::draw_text(sf::RenderTarget &w, string v, point p, int fs, bool ul) {
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
  text.setFillColor(sf::Color(200,200,200));
  text.setScale(inv);
  w.draw(text);
};

void graphics::draw_framed_text(sf::RenderTarget &w, string v, sf::FloatRect bounds, sf::Color co, sf::Color cf) {
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

sf::Image graphics::ship_image(string ship_class, float width, float height, sf::Color col) {
  return ship_image_label("", ship_class, width, height, col, col);
}

sf::Image graphics::ship_image_label(string text, string ship_class, float width, float height, sf::Color l_col, sf::Color s_col) {
  sf::RenderTexture tex;
  if (!tex.create(width, height)) {
    throw runtime_error("Failed to create render texture!");
  }

  tex.clear();

  ship s(ship::table().at(ship_class));
  s.position = point(width/2, height/2);
  s.angle = 0;
  float scale = width / 6;
  draw_ship(tex, s, s_col, scale);

  if (text.length() > 0) {
    int fs = 0.6 * fmin(width / (float)text.length(), height);
    draw_text(tex, text, s.position, fs);
  }
  
  tex.display();

  return tex.getTexture().copyToImage();
}

sfg::Button::Ptr graphics::ship_button(string ship_class, float width, float height, sf::Color col) {
  sfg::Button::Ptr b = sfg::Button::Create();
  b -> SetImage(sfg::Image::Create(ship_image(ship_class, width, height, col)));
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
point graphics::ul_corner(sf::RenderTarget &w){
  sf::View sv = w.getView();
  point v = sv.getSize();
  return sv.getCenter() - point(v.x * 0.5, v.y * 0.5);
}

// transform from pixels to points
sf::Transform graphics::view_inverse_transform(sf::RenderTarget &w){
  sf::Transform t;
  sf::View v = w.getView();

  t.translate(ul_corner(w));
  t.scale(v.getSize().x / w.getSize().x, v.getSize().y / w.getSize().y);
  return t;
}

// scale from pixels to points
point graphics::inverse_scale(sf::RenderTarget &w){
  sf::View v = w.getView();
  return point(v.getSize().x / w.getSize().x, v.getSize().y / w.getSize().y);
}

void graphics::draw_animation(sf::RenderTarget &w, animation e){
  float t = e.time_passed();
  sf::Color c(e.color);
  
  if (e.cat == animation_data::category::explosion) {
    float rad = e.magnitude * t * exp(-pow(3 * t,2));
    c = fade_color(c, sf::Color::White, 0.5);
    c.a = 100;
  
    sf::CircleShape s(rad);
    s.setFillColor(c);
    s.setPosition(e.p1 - point(rad, rad));
    w.draw(s);
  } else if (e.cat == animation_data::category::shield) {
    float rad = 10;
    c = fade_color(sf::Color::Blue, sf::Color::White, 0.5);
    c.a = 50 * t * exp(-pow(3 * t,2));
  
    sf::CircleShape s(rad);
    s.setFillColor(c);
    s.setPosition(e.p1 - point(rad, rad));
    w.draw(s);
  } else if (e.cat == animation_data::category::shot) {
    vector<sf::Vertex> svert;
    c.a = e.magnitude * exp(-pow(3 * t,2));

    svert.resize(2);
    svert[0].position = e.p1;
    svert[0].color = c;
    svert[1].position = e.p2;
    svert[1].color = c;

    w.draw(&svert[0], svert.size(), sf::LinesStrip);
  }
}
