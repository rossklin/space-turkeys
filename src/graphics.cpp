#include "graphics.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <string>

#include "animation.h"
#include "client_game.h"
#include "cost.h"
#include "ship.h"
#include "types.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace graphics;
using namespace client;

sf::Font graphics::default_font;
const float graphics::ship_scale_factor = 1;

const sf::Color graphics::solar_neutral(150, 150, 150);
const sf::Color graphics::command_selected_head(255, 255, 255, 255);
const sf::Color graphics::command_selected_body(150, 240, 150, 230);
const sf::Color graphics::command_selected_tail(80, 200, 80, 130);
const sf::Color graphics::command_selected_text(200, 200, 240);
const sf::Color graphics::command_normal_head(255, 255, 255, 200);
const sf::Color graphics::command_normal_body(100, 200, 100, 200);
const sf::Color graphics::command_normal_tail(50, 100, 50, 100);
const sf::Color graphics::command_normal_text(200, 200, 200, 200);
const sf::Color graphics::solar_fill(10, 20, 30, 40);
const sf::Color graphics::solar_selected(255, 255, 255, 180);
const sf::Color graphics::solar_selected_fill(200, 225, 255, 80);
const sf::Color graphics::fleet_fill(200, 200, 200, 50);
const sf::Color graphics::fleet_outline(40, 60, 180, 200);

float graphics::unscale() {
  return inverse_scale(g->window).x;
}

sf::Color graphics::fade_color(sf::Color from, sf::Color to, float r) {
  return sf::Color(from.r + r * (to.r - from.r),
                   from.g + r * (to.g - from.g),
                   from.b + r * (to.b - from.b),
                   from.a + r * (to.a - from.a));
}

void graphics::initialize() {
  bool loaded = default_font.loadFromFile("fonts/AjarSans-Regular.ttf");
  if (!loaded) loaded = default_font.loadFromFile(utility::root_path + "/fonts/AjarSans-Regular.ttf");

  if (!loaded) {
    throw classified_error("error loading font");
  }
}

void graphics::draw_flag(sf::RenderTarget &w, point p, sf::Color c, sf::Color bg, int count, string ship_class, int nstack) {
  vector<sf::Vertex> svert;
  float s = 35 * unscale();
  float bt = 0.05;

  // flag
  auto make_flag = [bt, c, bg](float shift) -> sf::ConvexShape {
    sf::ConvexShape flag;
    flag.setPointCount(3);
    flag.setPoint(0, point(bt / 2 + shift, -2 - shift));
    flag.setPoint(1, point(1 + shift, -1.5 - shift));
    flag.setPoint(2, point(bt / 2 + shift, -1 - shift));
    flag.setFillColor(shift > 0 ? sf::Color(200, 200, 200) : bg);
    flag.setOutlineColor(c);
    flag.setOutlineThickness(bt);
    return flag;
  };

  // pole
  sf::RectangleShape pole = build_rect(sf::FloatRect(-bt / 2, -1, bt, 1), 0, sf::Color::Transparent, c);

  sf::Transform t;
  t.translate(p.x, p.y);
  t.scale(s, s);

  for (int i = nstack - 1; i >= 0; i--) w.draw(make_flag(3 * i * bt), t);
  w.draw(pole, t);

  // draw ship symbol
  ship_ptr sh(new ship(ship::table().at(ship_class)));
  sh->position = p + s * point(0.5, -1.5);
  sh->angle = 0;
  draw_ship(w, sh, sf::Color::Black, 0.35 * s, false);
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

void graphics::draw_text(sf::RenderTarget &w, string v, point p, float fs, bool ul, sf::Color fill, bool do_inv, float rotate) {
  sf::Text text;
  float charsize = 16;
  float scale = fs / charsize;
  if (do_inv) scale *= unscale();
  text.setString(v);
  text.setFont(graphics::default_font);
  text.setCharacterSize(charsize);
  sf::FloatRect text_dims = text.getLocalBounds();
  if (ul) {
    text.setOrigin(point(text_dims.left, text_dims.top));
  } else {
    text.setOrigin(point(text_dims.left + text_dims.width / 2, text_dims.top + text_dims.height / 2));
  }
  text.setPosition(p);
  text.setFillColor(fill);
  text.setScale(point(scale, scale));
  text.setRotation(rotate * 360 / (2 * M_PI));
  w.draw(text);
};

sf::RectangleShape graphics::build_rect(sf::FloatRect bounds, float thickness, sf::Color co, sf::Color cf) {
  sf::RectangleShape r;
  r.setSize(sf::Vector2f(bounds.width, bounds.height));
  r.setPosition(sf::Vector2f(bounds.left, bounds.top));
  r.setOutlineColor(co);
  r.setFillColor(cf);
  r.setOutlineThickness(thickness);
  return r;
}

void graphics::draw_framed_text(sf::RenderTarget &w, string v, sf::FloatRect bounds, sf::Color co, sf::Color cf, float fs) {
  float margin = unscale() * 1;
  sf::RectangleShape r = graphics::build_rect(bounds, margin, co, cf);
  w.draw(r);

  if (fs == 0) {
    float h_limit = bounds.height - 2 * margin;
    float w_limit = (bounds.width - 2 * margin) / (v.length() * 1.2);
    fs = fmin(h_limit, w_limit);
  }

  point p(bounds.left + bounds.width / 2, bounds.top + bounds.height / 2);
  draw_text(w, v, p, fs, false, co, false);
}

sf::Image graphics::ship_image(string ship_class, float width, float height, sf::Color col) {
  return ship_image_label("", ship_class, width, height, col, col);
}

sf::Image graphics::ship_image_label(string text, string ship_class, float width, float height, sf::Color l_col, sf::Color s_col) {
  sf::RenderTexture tex;
  if (!tex.create(width, height)) {
    throw classified_error("Failed to create render texture!");
  }

  tex.clear();

  ship_ptr s(new ship(ship::table().at(ship_class)));
  s->position = point(width / 2, height / 2);
  s->angle = 0;
  float scale = width / 6;
  draw_ship(tex, s, s_col, scale);

  if (text.length() > 0) {
    int fs = 0.6 * fmin(width / (float)text.length(), height);
    draw_text(tex, text, s->position, fs);
  }

  tex.display();

  return tex.getTexture().copyToImage();
}

sfg::Button::Ptr graphics::ship_button(string ship_class, float width, float height, sf::Color col) {
  sfg::Button::Ptr b = sfg::Button::Create();
  b->SetImage(sfg::Image::Create(ship_image(ship_class, width, height, col)));
  return b;
};

void graphics::draw_ship(sf::RenderTarget &w, ship_ptr s, sf::Color col, float sc, bool multicolor) {
  vector<sf::Vertex> svert;

  sf::Color cnose(255, 200, 180, 200);

  svert.resize(s->shape.size());
  for (int i = 0; i < s->shape.size(); i++) {
    svert[i].position = s->shape[i].first;
    svert[i].color = s->shape[i].second == 'p' || !multicolor ? col : cnose;
  }

  sf::Transform t;
  t.translate(s->position.x, s->position.y);
  t.rotate(s->angle / (2 * M_PI) * 360);
  t.scale(sc, sc);
  w.draw(&svert[0], svert.size(), sf::LinesStrip, t);

  // temp for debug
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::L)) {
    draw_text(w, s->id, s->position, 12, false);
  }
}

// point coordinates of view ul corner
point graphics::ul_corner(sf::RenderTarget &w) {
  sf::View sv = w.getView();
  point v = sv.getSize();
  return sv.getCenter() - point(v.x * 0.5, v.y * 0.5);
}

// transform from pixels to points
sf::Transform graphics::view_inverse_transform(sf::RenderTarget &w) {
  sf::Transform t;
  sf::View v = w.getView();

  t.translate(ul_corner(w));
  t.scale(v.getSize().x / w.getSize().x, v.getSize().y / w.getSize().y);
  return t;
}

// scale from pixels to points
point graphics::inverse_scale(sf::RenderTarget &w) {
  sf::View v = w.getView();
  return point(v.getSize().x / w.getSize().x, v.getSize().y / w.getSize().y);
}

void graphics::draw_animation(sf::RenderTarget &w, animation e) {
  float t = e.time_passed();
  if (t < 0) return;

  sf::Color c(e.color);
  int alpha_wave = 1000 * t * exp(-pow(3 * t, 2));

  auto fexpl = [&w, e, t, c, alpha_wave](sf::Color ct) {
    float rad = sqrt(e.magnitude) * t;
    sf::Color col = fade_color(c, ct, t / animation::tmax);
    col.a = alpha_wave;

    sf::CircleShape s(rad);
    s.setFillColor(col);
    s.setPosition(e.t1.p - point(rad, rad));
    w.draw(s);
  };

  if (e.cat == animation_data::category::explosion) {
    fexpl(sf::Color::White);
  } else if (e.cat == animation_data::category::bomb) {
    fexpl(sf::Color::Red);
  } else if (e.cat == animation_data::category::shield) {
    float rad = ship_scale_factor * e.radius;
    c = sf::Color(130, 130, 255);
    c.a = alpha_wave;

    sf::CircleShape s(rad);
    s.setFillColor(c);
    s.setPosition(e.t1.p - point(rad, rad));
    w.draw(s);
  } else if (e.cat == animation_data::category::shot) {
    vector<sf::Vertex> svert;
    c.a = utility::sigmoid(60 * e.magnitude * exp(-pow(7 * t, 2)), 255);

    svert.resize(2);
    svert[0].position = e.t1.p;
    svert[0].color = c;
    svert[1].position = e.t2.p;
    svert[1].color = c;

    w.draw(&svert[0], svert.size(), sf::LinesStrip);
  } else if (e.cat == animation_data::category::message) {
    float fs = graphics::unscale() * 16;
    float width = e.text.length() * fs * 0.6;
    float height = 1.3 * fs;
    point p = e.t1.p + point(10, -height / 2);
    sf::FloatRect bounds(p, point(width, height));
    sf::Color co = sf::Color::White;
    sf::Color cf(40, 40, 40, 180);
    draw_framed_text(w, e.text, bounds, co, cf, fs);
  }
}

sf::Image graphics::selector_card(string title, bool available, float progress) {
  sf::RenderTexture tex;
  int width = 120;
  int height = 200;
  sf::FloatRect bounds(0, 0, width, height);

  cout << "selector card: " << title << ": progress = " << progress << endl;

  if (!tex.create(width, height)) {
    throw classified_error("Failed to create render texture!");
  }

  tex.clear();

  // draw progress
  auto pfill = build_rect(sf::FloatRect(0, (1 - progress) * bounds.height, bounds.width, progress * bounds.height), 0, sf::Color::Transparent, sf::Color(50, 50, 80));
  if (progress < 0) {
    // already complete
    pfill = build_rect(sf::FloatRect(0, 0, bounds.width, bounds.height), 0, sf::Color::Transparent, sf::Color(50, 100, 50));
  }
  tex.draw(pfill);

  // draw text
  draw_text(tex, title, point(width / 2, height / 2), 16, false, sf::Color::White, true, M_PI / 2);

  // draw outline
  sf::Color outline = sf::Color(150, 150, 150);
  tex.draw(build_rect(bounds, -2, outline));

  // draw available
  if (!available) {
    auto pgrey = build_rect(bounds, 0, sf::Color::Transparent, sf::Color(120, 120, 120, 120));
    tex.draw(pgrey);
  }

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
  point req = w->GetRequisition();

  if (horizontal) {
    sw->SetRequisition(point(dim, req.y));
  } else {
    sw->SetRequisition(point(req.x, dim));
  }

  return sw;
}

sfg::Widget::Ptr graphics::wrap_in_scroll2(sfg::Widget::Ptr w, int width, int height) {
  sfg::ScrolledWindow::Ptr sw = sfg::ScrolledWindow::Create();
  sw->SetScrollbarPolicy(sfg::ScrolledWindow::HORIZONTAL_AUTOMATIC | sfg::ScrolledWindow::VERTICAL_AUTOMATIC);
  sw->AddWithViewport(w);
  sw->SetRequisition(point(width, height));
  return sw;
}
