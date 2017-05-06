#include <iostream>

#include "types.h"
#include "selector.h"
#include "solar.h"
#include "fleet.h"
#include "utility.h"
#include "graphics.h"
#include "client_game.h"

using namespace std;

st3::client::game *st3::client::entity_selector::g = 0;

namespace st3{    
  namespace client{
    template class specific_selector<ship>;
    template class specific_selector<fleet>;
    template class specific_selector<solar>;
    template class specific_selector<waypoint>;

    // solar selector
    template<>
    bool specific_selector<solar>::contains_point(point p, float &d){
      d = utility::l2norm(p - position);
      return d < radius;
    }

    template<>
    bool specific_selector<solar>::is_selectable() {
      return true;
    }

    template<>
    void specific_selector<solar>::draw(window_t &w){
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
	sol.setRadius(vision());
	sol.setPointCount(vision() / graphics::inverse_scale(w).x);
	sol.setFillColor(sf::Color::Transparent);
	sol.setOutlineThickness(-1);
	sol.setOutlineColor(sf::Color(40, 200, 60, 100));
	sol.setPosition(position - point(vision(), vision()));
	w.draw(sol);
	w.draw(text);
      }

      // draw defense indicator
      float defrad = radius + 4; 
      sol.setRadius(defrad);
      sol.setPointCount(defrad / graphics::inverse_scale(w).x);
      sol.setOutlineThickness(utility::sigmoid(turrets.size(), 100) / 20);
      sol.setOutlineColor(sf::Color(100, 140, 200, 150));
      sol.setFillColor(sf::Color::Transparent);
      sol.setPosition(position - point(defrad, defrad));
      w.draw(sol);
    }

    template<>
    set<combid> specific_selector<solar>::get_ships(){return ships;}

    template<>
    string specific_selector<solar>::hover_info(){
      string res = "solar at " + utility::format_float(position.x) + "x" + utility::format_float(position.y);

      for (auto k : cost::keywords::resource)
	res += "\nres:" + k + ": " + to_string((int)resource[k].available);

      if (owned){
	res += "\npopulation: " + to_string((int)population);
	res += "\nfleet: " + to_string(ships.size());
	res += "\nturrets: " + to_string(turrets.size());
      }

      return res;
    }

    // ****************************************
    // FLEET SELECTOR
    // ****************************************

    template<>
    bool specific_selector<fleet>::contains_point(point p, float &d){
      d = utility::l2norm(p - position);
      return d < radius;
    }

    template<>
    bool specific_selector<fleet>::is_selectable() {
      return true;
    }

    template<>
    void specific_selector<fleet>::draw(window_t &w){
      auto f = [this, &w] (float r, sf::Color cf, sf::Color co) {
	sf::CircleShape s(r);
	s.setPointCount(r / graphics::inverse_scale(w).x);
	s.setFillColor(cf);
	s.setOutlineColor(co);
	s.setOutlineThickness(1);
	s.setPosition(position - point(r, r));
	w.draw(s);
      };

      if (owned){      
	if (selected){
	  f(radius, graphics::fleet_fill, graphics::fleet_outline);
	}
	f(vision(), sf::Color::Transparent, sf::Color(40, 200, 60, 100));
      }else{
	f(radius, sf::Color::Transparent, sf::Color(200, 40, 60, 100));
      }
    }

    template<>
    set<combid> specific_selector<fleet>::get_ships(){
      return ships;
    }

    template<>
    string specific_selector<fleet>::hover_info(){
      string res = "fleet at " + utility::format_float(position.x) + "x" + utility::format_float(position.y);

      if (owned){
	res += "\nships: " + to_string(ships.size());
	res += "\naction: " + com.action;
      }

      return res;
    }

    // ****************************************
    // WAYPOINT SELECTOR
    // ****************************************

    template<>
    bool specific_selector<waypoint>::contains_point(point p, float &d){
      d = utility::l2norm(p - position);
      return d < radius;
    }

    template<>
    bool specific_selector<waypoint>::is_selectable() {
      return true;
    }

    template<>
    void specific_selector<waypoint>::draw(window_t &w){
      sf::CircleShape s(radius);
      s.setFillColor(selected ? sf::Color(255,255,255,100) : sf::Color(0,0,0,0));
      s.setOutlineColor(get_color());
      s.setOutlineThickness(1);
      s.setPosition(position - point(radius, radius));
      w.draw(s);
    }

    template<>
    set<combid> specific_selector<waypoint>::get_ships(){
      set<combid> s;
      for (auto i : g -> incident_commands(id)) {
	s += g -> command_selectors[i] -> ships;
      }
      return s;
    }

    template<>
    string specific_selector<waypoint>::hover_info(){
      return "waypoint at " + utility::format_float(position.x) + "x" + utility::format_float(position.y);
    }

    // ****************************************
    // SHIP SELECTOR
    // ****************************************

    template<>
    bool specific_selector<ship>::contains_point(point p, float &d){
      static float rad = 10;
      d = utility::l2norm(p - position);
      return d < rad;
    }

    template<>
    bool specific_selector<ship>::is_selectable() {
      return !has_fleet();
    }

    template<>
    void specific_selector<ship>::draw(window_t &w){
      if (!is_active()) return;

      if (selected) {
	float rad = 5;
	sf::CircleShape s(rad);
	s.setFillColor(sf::Color(255,255,255,50));
	s.setPosition(position - point(rad, rad));
	w.draw(s);
      }
      
      graphics::draw_ship(w, *this, get_color());
    }

    template<>
    set<combid> specific_selector<ship>::get_ships(){
      return set<combid>();
    }

    template<>
    string specific_selector<ship>::hover_info(){
      return ship_class + " ship at " + utility::format_float(position.x) + "x" + utility::format_float(position.y);
    }
  }
}

using namespace st3;
using namespace st3::client;

// ****************************************
// ENTITY SELECTOR
// ****************************************

entity_selector::entity_selector(sf::Color c, bool o){
  color = c;
  owned = o;
  seen = owned;
  selected = false;
  queue_level = 0;
}

entity_selector::~entity_selector(){}

bool entity_selector::is_area_selectable() {
  return this -> is_selectable() && !this -> isa(waypoint::class_id);
}

sf::Color entity_selector::get_color(){
  return seen ? color : graphics::fade_color(color, sf::Color(150,150,150,10), 0.6);
}

bool entity_selector::inside_rect(sf::FloatRect r){
  return r.contains(get_position());
}

// ****************************************
// SPECIFIC SELECTOR
// ****************************************

template<typename T>
specific_selector<T>::specific_selector(T &s, sf::Color c, bool o) : entity_selector(c,o), T(s) {
  this->copy_from(s);
}

template<typename T>
specific_selector<T>::~specific_selector() {}

template<typename T>
point specific_selector<T>::get_position(){
  return position;
}

template<typename T>
typename specific_selector<T>::ptr specific_selector<T>::create(T &s, sf::Color c, bool o){
  return typename specific_selector<T>::ptr(new specific_selector<T>(s, c, o));
}


// ****************************************
// COMMAND SELECTOR
// ****************************************

command_selector::ptr command_selector::create(command c, point s, point d){
  return ptr(new command_selector(c, s, d));
}

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
