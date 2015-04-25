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
  area_selectable = true;
  selected = false;
}

entity_selector::~entity_selector(){}

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
  text.setFont(graphics::default_font); 
  text.setString(to_string(ships.size()));
  text.setCharacterSize(14);
  // text.setStyle(sf::Text::Underlined);
  sf::FloatRect text_dims = text.getLocalBounds();
  text.setPosition(position - point(text_dims.width/2, text_dims.height));
  text.setColor(sf::Color(200,200,200));

  sf::CircleShape sol(radius);
  sol.setFillColor(graphics::solar_fill);
  sol.setOutlineThickness(-radius / 3);
  sol.setOutlineColor(color);
  sol.setPosition(position.x - radius, position.y - radius);
  w.draw(sol);
  
  if (selected){
    sol.setFillColor(sf::Color::Transparent);
    sol.setOutlineThickness(radius / 5);
    sol.setOutlineColor(graphics::solar_selected);
    w.draw(sol);
  }

  w.draw(text);
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
    s.setFillColor(graphics::fleet_fill);
    s.setOutlineColor(graphics::fleet_outline);
    s.setOutlineThickness(2);
    s.setPosition(position);
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
// COMMAND SELECTOR
// ****************************************

command_selector::command_selector(command &c, point s, point d) : entity_selector(graphics::command_normal_body, true), command(c){
  from = s;
  to = d;
  area_selectable = false;
}

command_selector::~command_selector(){}

bool command_selector::contains_point(point p, float &d){
  d = utility::dpoint2line(p, from, get_position());

  // todo: command size?
  return d < 5;
}

point command_selector::get_position(){
  return to;
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

bool command_selector::isa(string t){
  return !t.compare(identifier::command);
}

set<idtype> command_selector::get_ships(){
  return ships;
}
