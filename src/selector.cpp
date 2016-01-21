#include <iostream>

#include "types.h"
#include "selector.h"
#include "solar.h"
#include "fleet.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace st3::client;

// ****************************************
// ENTITY SELECTOR
// ****************************************

entity_selector::entity_selector(sf::Color c, bool o){
  color = c;
  owned = o;
  seen = owned;
  area_selectable = true;
  selected = false;
  queue_level = 0;
}

entity_selector::~entity_selector(){}

sf::Color entity_selector::get_color(){
  return seen ? color : graphics::fade_color(color, sf::Color(150,150,150,10), 0.6);
}

bool entity_selector::inside_rect(sf::FloatRect r){
  return r.contains(get_position());
}

// ****************************************
// SOLAR SELECTOR
// ****************************************

solar_selector::solar_selector(solar &s, sf::Color c, bool o) : entity_selector(c,o), solar(s){}

solar_selector::~solar_selector() {}

bool solar_selector::contains_point(point p, float &d){
  d = utility::l2norm(p - position);
  return d < radius;
}

point solar_selector::get_position(){
  return position;
}

void solar_selector::draw(window_t &w){
  // setup text
  sf::Text text;
  text.setString(to_string(ships.size()));
  text.setFont(graphics::default_font); 
  text.setCharacterSize(24);
  sf::FloatRect text_dims = text.getLocalBounds();
  text.setOrigin(point(text_dims.left + text_dims.width/2, text_dims.top + text_dims.height / 2));
  text.setPosition(position); 
  text.setColor(sf::Color(200,200,200));
  text.setScale(graphics::inverse_scale(w));

  sf::CircleShape sol(radius);
  // compute fill color
  sf::Color cfill;
  cfill.r = 256 * utility::sigmoid(resource[cost::keywords::key_metals].available / 1000);
  cfill.g = 256 * utility::sigmoid(resource[cost::keywords::key_organics].available / 1000);
  cfill.b = 256 * utility::sigmoid(resource[cost::keywords::key_gases].available / 1000);
  cfill.a = 100;
  sol.setPointCount(radius / graphics::inverse_scale(w).x);
  sol.setFillColor(cfill);
  sol.setOutlineThickness(-1);
  sol.setOutlineColor(get_color());
  sol.setPosition(position.x - radius, position.y - radius);
  w.draw(sol);
  
  if (selected){
    sol.setFillColor(graphics::solar_selected_fill);
    sol.setOutlineThickness(1);
    sol.setOutlineColor(graphics::solar_selected);
    w.draw(sol);
  }

  if (owned){
    sol.setRadius(compute_vision());
    sol.setPointCount(compute_vision() / graphics::inverse_scale(w).x);
    sol.setFillColor(sf::Color::Transparent);
    sol.setOutlineThickness(-1);
    sol.setOutlineColor(sf::Color(40, 200, 60, 100));
    sol.setPosition(position - point(compute_vision(), compute_vision()));
    w.draw(sol);
    w.draw(text);
  }

  // draw defense indicator
  float defrad = radius + 4; 
  sol.setRadius(defrad);
  sol.setPointCount(defrad / graphics::inverse_scale(w).x);
  sol.setOutlineThickness(utility::sigmoid(has_defense(), 100) / 20);
  sol.setOutlineColor(sf::Color(100, 140, 200, 150));
  sol.setFillColor(sf::Color::Transparent);
  sol.setPosition(position - point(defrad, defrad));
  w.draw(sol);
}

bool solar_selector::isa(string t){
  return !t.compare(identifier::solar);
}

set<idtype> solar_selector::get_ships(){
  return ships;
}

// ****************************************
// FLEET SELECTOR
// ****************************************

fleet_selector::fleet_selector(fleet &f, sf::Color c, bool o) : entity_selector(c, o), fleet(f){}

fleet_selector::~fleet_selector(){}

bool fleet_selector::contains_point(point p, float &d){
  d = utility::l2norm(p - position);
  return d < radius;
}

point fleet_selector::get_position(){
  return position;
}

void fleet_selector::draw(window_t &w){
  if (selected){
    sf::CircleShape s(radius);
    s.setPointCount(radius / graphics::inverse_scale(w).x);
    s.setFillColor(graphics::fleet_fill);
    s.setOutlineColor(graphics::fleet_outline);
    s.setOutlineThickness(1);
    s.setPosition(position - point(radius, radius));
    w.draw(s);
  }

  if (owned){
    sf::CircleShape s(vision);
    s.setPointCount(vision / graphics::inverse_scale(w).x);
    s.setFillColor(sf::Color::Transparent);
    s.setOutlineColor(sf::Color(40, 200, 60, 100));
    s.setOutlineThickness(-1);
    s.setPosition(position - point(vision, vision));
    w.draw(s);
  }
}

bool fleet_selector::isa(string t){
  return !t.compare(identifier::fleet);
}

set<idtype> fleet_selector::get_ships(){
  return ships;
}

// ****************************************
// WAYPOINT SELECTOR
// ****************************************

waypoint_selector::waypoint_selector(waypoint &f, sf::Color c) : entity_selector(c, true), waypoint(f){}

waypoint_selector::~waypoint_selector(){}

bool waypoint_selector::contains_point(point p, float &d){
  d = utility::l2norm(p - position);
  return d < radius;
}

point waypoint_selector::get_position(){
  return position;
}

void waypoint_selector::draw(window_t &w){
  sf::CircleShape s(radius);
  s.setFillColor(selected ? sf::Color(255,255,255,100) : sf::Color(0,0,0,0));
  s.setOutlineColor(get_color());
  s.setOutlineThickness(1);
  s.setPosition(position - point(radius, radius));
  w.draw(s);
}

bool waypoint_selector::isa(string t){
  return !t.compare(identifier::waypoint);
}

set<idtype> waypoint_selector::get_ships(){
  return ships;
}

// ****************************************
// COMMAND SELECTOR
// ****************************************

command_selector::command_selector(command &c, point s, point d) : command(c){
  from = s;
  to = d;
  queue_level = 0;
  selected = false;
}

command_selector::~command_selector(){}

bool command_selector::contains_point(point p, float &d){
  d = utility::dpoint2line(p, from, to);

  // todo: command size?
  return d < 5;
}

void command_selector::draw(window_t &w){
  static sf::Vertex c_head[3] = {
    sf::Vertex(sf::Vector2f(1, 0)),
    sf::Vertex(sf::Vector2f(0.9, -3)),
    sf::Vertex(sf::Vector2f(0.9, 3))
  };

  static sf::Vertex c_body[3] = {
    sf::Vertex(sf::Vector2f(0.9, 2)),
    sf::Vertex(sf::Vector2f(0.9, -2)),
    sf::Vertex(sf::Vector2f(0, 0))
  };
  
  // setup text
  sf::Text text;
  text.setFont(graphics::default_font); 
  text.setString(to_string(ships.size()));
  text.setCharacterSize(14);
  // text.setStyle(sf::Text::Underlined);
  sf::FloatRect text_dims = text.getLocalBounds();
  text.setPosition(utility::scale_point(to + from, 0.5) - point(text_dims.width/2, text_dims.height + 10));
  text.setScale(graphics::inverse_scale(w));

  // setup arrow colors
  if (selected){
    text.setColor(graphics::command_selected_text);
    c_head[0].color = graphics::command_selected_head;
    c_head[1].color = graphics::command_selected_body;
    c_head[2].color = graphics::command_selected_body;
    c_body[0].color = graphics::command_selected_body;
    c_body[1].color = graphics::command_selected_body;
    c_body[2].color = graphics::command_selected_tail;
  }else{
    text.setColor(graphics::command_normal_text);
    c_head[0].color = graphics::command_normal_head;
    c_head[1].color = graphics::command_normal_body;
    c_head[2].color = graphics::command_normal_body;
    c_body[0].color = graphics::command_normal_body;
    c_body[1].color = graphics::command_normal_body;
    c_body[2].color = graphics::command_normal_tail;
  }

  // setup arrow transform
  sf::Transform t;
  point delta = to - from;
  float scale = utility::l2norm(delta);
  t.translate(from);
  t.rotate(utility::point_angle(delta) / (2 * M_PI) * 360);
  t.scale(scale, 1);

  w.draw(c_head, 3, sf::Triangles, t);
  w.draw(c_body, 3, sf::Triangles, t);
  w.draw(text);

}
