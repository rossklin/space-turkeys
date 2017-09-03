#include <iostream>

#include "types.h"
#include "selector.h"
#include "solar.h"
#include "fleet.h"
#include "utility.h"
#include "graphics.h"
#include "client_game.h"

using namespace std;

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
    set<combid> specific_selector<solar>::get_ships(){return ships;}

    template<>
    void specific_selector<solar>::draw(window_t &w){
      // compute fill color
      sf::Color cfill;
      cfill.r = 256 * utility::sigmoid(2 * available_resource[keywords::key_metals] / 1000);
      cfill.g = 256 * utility::sigmoid(2 * available_resource[keywords::key_organics] / 1000);
      cfill.b = 256 * utility::sigmoid(2 * available_resource[keywords::key_gases] / 1000);
      cfill.a = 160;
      graphics::draw_circle(w, position, radius, get_color(), cfill, -2);

      string indicator_text = "";
      if (!was_discovered) indicator_text += "!";

      if (owned){
	graphics::draw_circle(w, position, vision(), sf::Color(40, 200, 60, 100));
	graphics::draw_text(w, to_string((int)(population / 1000)), position, 16);

	indicator_text = choice_data.governor.substr(0, 2);
	if (get_ships().size()) indicator_text += " <>";
  
	if (selected){
	  graphics::draw_circle(w, position, radius + 1, graphics::solar_selected, graphics::solar_selected_fill, 2);

	  hm_t<string, int> ship_counts;
	  for (auto sid : get_ships()) ship_counts[g -> get_specific<ship>(sid) -> ship_class]++;

	  if (ship_counts.size()) {
	    
	    string res = "";
	    int maxlen = 0;
	    for (auto v : ship_counts) {
	      string buf = to_string(v.second) + " " + v.first + "s";
	      if (maxlen) buf = "\n" + buf;
	      maxlen = max((int)buf.length(), maxlen);
	      res += buf;
	    }

	    float fs = graphics::unscale() * 16;
	    int n = ship_counts.size();
	    float width = maxlen * fs * 0.6;
	    float height = 1.3 * n * fs;
	    sf::FloatRect bounds(position.x + radius + 10, position.y - height / 2, width, height);
	    graphics::draw_framed_text(w, res, bounds, sf::Color::White, sf::Color(20, 30, 40, 80), fs);
	  }
	}
      }

      // draw defense indicator
      float sp = compute_shield_power();
      if (sp > 0) {
	graphics::draw_circle(w, position, radius + 4, sf::Color(100, 140, 200, 150), sf::Color::Transparent, sp);
      }

      // draw symbol indicators
      if (indicator_text.length()) {
	graphics::draw_text(w, indicator_text, position + point(radius, -radius), 14);
      }
    }

    template<>
    string specific_selector<solar>::hover_info(){
      string res = id + " at " + utility::format_float(position.x) + "x" + utility::format_float(position.y);

      if (owned) {
	res += "\n<<Status>>";
	res += "\npopulation: " + to_string((int)population);
	res += "\nhappiness: " + to_string((int)(100 * happiness)) + "%";
	res += "\necology: " + to_string((int)(100 * ecology)) + "%";
	res += "\ncrowding: " + to_string((int)(100 * crowding_rate() / population)) + "%";
	res += "\nshield: " + to_string(compute_shield_power());

	res += "\n<<Facilities>>: ";
	for (auto x : facility_table()) {
	  facility_object f = developed(x.first);
	  bool is_active = x.first == choice_data.development;
	  if (f.level == 0 && !is_active) continue;
	  
	  res += "\n" + x.first + ": " + to_string(f.level);
	  
	  if (is_active) {
	    f = developed(x.first, 1);
	    res += " (" + to_string((int)(100 * f.progress / f.cost_time)) + "%)";
	  }
	}

	if (fleet_growth.count()) {
	  res += "\n<<Shipyard>>";
	  for (auto x : fleet_growth.data) {
	    if (x.second > 0) {
	      ship_stats s = ship::table().at(x.first);
	      res += "\n" + x.first + ": " + to_string((int)(100 * x.second / s.build_time)) + "%";
	    }
	  }
	}
      }

      res += "\n<<Resources>>:";
      for (auto k : keywords::resource) {
	res += "\n" + k + ": " + to_string((int)available_resource[k]);
	if (owned) res += " (storage: " + to_string((int)resource_storage[k]) + ")";
      }

      return res;
    }

    // ****************************************
    // FLEET SELECTOR
    // ****************************************

    template<>
    bool specific_selector<fleet>::contains_point(point p, float &d){
      d = utility::l2norm(p - position) / graphics::unscale();
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
	s.setPointCount(r / graphics::unscale());
	s.setFillColor(cf);
	s.setOutlineColor(co);
	s.setOutlineThickness(graphics::unscale());
	s.setPosition(position - point(r, r));
	w.draw(s);
      };

      sf::Color outline = graphics::fleet_outline;
      if (selected) outline = graphics::fade_color(outline, sf::Color::White, 0.4);
      f(radius * graphics::unscale(), graphics::fleet_fill, outline);
      f(vision(), sf::Color::Transparent, sf::Color(40, 200, 60, 50));

      // add a flag
      graphics::draw_flag(w, position, graphics::unscale(), get_color());
    }

    template<>
    set<combid> specific_selector<fleet>::get_ships(){
      return ships;
    }

    template<>
    string specific_selector<fleet>::hover_info(){
      string res = "fleet at " + utility::format_float(position.x) + "x" + utility::format_float(position.y);

      if (owned){
	hm_t<string, int> ship_counts;
	for (auto sid : get_ships()) ship_counts[g -> get_specific<ship>(sid) -> ship_class]++;

	res += "\naction: " + com.action;
	for (auto x : ship_counts) res += "\n" + x.first + "s: " + to_string(x.second);
      }

      return res;
    }

    // ****************************************
    // WAYPOINT SELECTOR
    // ****************************************

    template<>
    bool specific_selector<waypoint>::contains_point(point p, float &d){
      d = utility::l2norm(p - position) / graphics::unscale();
      return d < radius;
    }

    template<>
    bool specific_selector<waypoint>::is_selectable() {
      return true;
    }

    template<>
    void specific_selector<waypoint>::draw(window_t &w){
      float r = radius * graphics::unscale();
      sf::CircleShape s(r);
      s.setFillColor(selected ? sf::Color(255,255,255,100) : sf::Color(0,0,0,0));
      s.setOutlineColor(get_color());
      s.setOutlineThickness(graphics::unscale());
      s.setPosition(position - point(r,r));
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
      d = utility::l2norm(p - position) / graphics::unscale();
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
      
      graphics::draw_ship(w, *this, get_color(), graphics::ship_scale_factor * radius);
    }

    template<>
    set<combid> specific_selector<ship>::get_ships(){
      return set<combid>();
    }

    template<>
    string specific_selector<ship>::hover_info(){
      auto get_percent = [this] (int k) -> int {
	return 100 * stats[k] / base_stats.stats[k];
      };

      auto build_percent = [this, get_percent] (string label, int percent) -> string {
	return label + ": " + to_string(percent) + "%" + "\n";
      };
      
      string output = ship_class + "\n";
      output += build_percent("Hull", get_percent(sskey::key::hp));
      output += build_percent("Weapons", 100 * load / stats[sskey::key::load_time]);
      
      if (base_stats.stats[sskey::key::shield] > 0) {
	output += build_percent("Shields", get_percent(sskey::key::shield));
      }

      if (upgrades.size()) {
	output += "Upgrades:\n";
	for (auto u : upgrades) output += u + "\n";
      }

      auto maybe_include = [this, &output] (string label, int value) {
	if (value > 0) output += label + ": " + to_string(value) + "\n";
      };

      maybe_include("Passengers", passengers);

      if (is_loaded) {
	if (cargo.count()) {
	  maybe_include("Cargo: metals", cargo[keywords::key_metals]);
	  maybe_include("Cargo: gases", cargo[keywords::key_gases]);
	  maybe_include("Cargo: organics", cargo[keywords::key_organics]);
	} else {
	  output += "Cargo: empty\n";
	}
      }

      return output;
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
  base_position = position;
  base_angle = 0;
}

template<>
specific_selector<ship>::specific_selector(ship &s, sf::Color c, bool o) : entity_selector(c,o), ship(s) {
  base_position = position;
  base_angle = angle;
}

template<typename T>
entity_selector::ptr specific_selector<T>::ss_clone() {
  return create(static_cast<T&>(*this), color, owned);
}

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
  static idtype idc = 0;
  id = idc++;
  from = s;
  to = d;
  queue_level = 0;
  selected = false;
}

command_selector::~command_selector(){}

bool command_selector::contains_point(point p, float &d){
  d = utility::dpoint2line(p, from, to) / graphics::unscale();

  // todo: command size?
  return d < 5;
}

void command_selector::draw(window_t &w){
  float body_length = 1 - 0.1 * graphics::unscale();

  sf::Vertex c_head[3] = {
    sf::Vertex(sf::Vector2f(1, 0)),
    sf::Vertex(sf::Vector2f(body_length, -3)),
    sf::Vertex(sf::Vector2f(body_length, 3))
  };

  sf::Vertex c_body[3] = {
    sf::Vertex(sf::Vector2f(body_length, 2)),
    sf::Vertex(sf::Vector2f(body_length, -2)),
    sf::Vertex(sf::Vector2f(0, 0))
  };
  
  // setup text
  sf::Text text;
  text.setFont(graphics::default_font); 
  text.setString(to_string(ships.size()));
  text.setCharacterSize(14);
  // text.setStyle(sf::Text::Underlined);
  sf::FloatRect text_dims = text.getLocalBounds();
  text.setPosition(utility::scale_point(to + from, 0.5) - utility::scale_point(point(text_dims.width/2, text_dims.height + 10), graphics::unscale()));
  text.setScale(graphics::inverse_scale(w));

  // setup arrow colors
  if (selected){
    text.setFillColor(graphics::command_selected_text);
    c_head[0].color = graphics::command_selected_head;
    c_head[1].color = graphics::command_selected_body;
    c_head[2].color = graphics::command_selected_body;
    c_body[0].color = graphics::command_selected_body;
    c_body[1].color = graphics::command_selected_body;
    c_body[2].color = graphics::command_selected_tail;
  }else{
    text.setFillColor(graphics::command_normal_text);
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
  t.scale(scale, graphics::unscale());

  w.draw(c_head, 3, sf::Triangles, t);
  w.draw(c_body, 3, sf::Triangles, t);
  w.draw(text);
}
