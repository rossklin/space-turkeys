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

void graphics::draw_text(sf::RenderTarget &w, string v, point p, int fs, bool ul, sf::Color fill) {
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
  text.setFillColor(fill);
  text.setScale(inv);
  w.draw(text);
};

sf::RectangleShape graphics::build_rect(sf::FloatRect bounds, int thickness, sf::Color co, sf::Color cf) {
  sf::RectangleShape r;
  r.setSize(sf::Vector2f(bounds.width, bounds.height));
  r.setPosition(sf::Vector2f(bounds.left, bounds.top));
  r.setOutlineColor(co);
  r.setFillColor(cf);
  r.setOutlineThickness(thickness);
  return r;
}

void graphics::draw_framed_text(sf::RenderTarget &w, string v, sf::FloatRect bounds, sf::Color co, sf::Color cf) {
  int margin = ceil(bounds.height / 10);
  sf::RectangleShape r = graphics::build_rect(bounds, -margin, co, cf);
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

sf::Image graphics::selector_card(string title, bool selected, float progress, list<string> info, list<string> requirements) {
  sf::RenderTexture tex;
  int width = 120;
  int height = 200;
  sf::FloatRect bounds(0, 0, width, height);
  
  if (!tex.create(width, height)) {
    throw runtime_error("Failed to create render texture!");
  }

  tex.clear();

  // draw progress
  auto pfill = build_rect(sf::FloatRect(0, (1 - progress) * bounds.height, bounds.width, progress * bounds.height), 0, sf::Color::Transparent, sf::Color(50,50,80));
  tex.draw(pfill);

  // draw text
  draw_text(tex, title, point(width / 2, 20), 16, false);
  int c = 2;
  for (auto v : requirements) draw_text(tex, v, point(width / 2, (c++) * 20), 11, false, sf::Color::Red);
  c++;
  for (auto v : info) draw_text(tex, v, point(width / 2, (c++) * 20), 11, false);

  // draw outline
  sf::Color outline = sf::Color(150,150,150);
  if (selected) outline = sf::Color::White;
  tex.draw(build_rect(bounds, -2, outline));
  
  tex.display();

  return tex.getTexture().copyToImage();  
}

sfg::Widget::Ptr graphics::wrap_in_scroll(sfg::Widget::Ptr w, bool horizontal, int dim) {
  sfg::ScrolledWindow::Ptr sw = sfg::ScrolledWindow::Create();
  if (horizontal) {
    sw->SetScrollbarPolicy(sfg::ScrolledWindow::HORIZONTAL_AUTOMATIC | sfg::ScrolledWindow::VERTICAL_NEVER);
  } else {
    sw->SetScrollbarPolicy(sfg::ScrolledWindow::HORIZONTAL_NEVER | sfg::ScrolledWindow::VERTICAL_AUTOMATIC);
  }
  
  sw->AddWithViewport(w);
  point req = w -> GetRequisition();

  if (horizontal) {
    sw->SetRequisition(point(dim, req.y));
  }else{
    sw->SetRequisition(point(req.x, dim));
  }

  return sw;
}
