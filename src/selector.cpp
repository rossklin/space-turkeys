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
	hm_t<string, string> gov_abr;
	gov_abr[keywords::key_military] = "MY";
	gov_abr[keywords::key_research] = "R";
	gov_abr[keywords::key_mining] = "MG";
	gov_abr[keywords::key_development] = "D";
	gov_abr[keywords::key_culture] = "C";
	
	graphics::draw_circle(w, position, vision(), sf::Color(40, 200, 60, 100));
	graphics::draw_text(w, to_string((int)(population / 1000)), position, 16);

	indicator_text = gov_abr[choice_data.governor];
	if (get_ships().size()) indicator_text += " <>";
  
	if (selected){
	  graphics::draw_circle(w, position, radius + 1, graphics::solar_selected, graphics::solar_selected_fill, 2);

	  auto counts = ship_counts();

	  if (counts.size()) {
	    
	    string res = "";
	    int maxlen = 0;
	    for (auto v : counts) {
	      string buf = to_string(v.second) + " " + v.first + "s";
	      if (maxlen) buf = "\n" + buf;
	      maxlen = max((int)buf.length(), maxlen);
	      res += buf;
	    }

	    float fs = graphics::unscale() * 16;
	    int n = counts.size();
	    float width = maxlen * fs * 0.6;
	    float height = 1.3 * n * fs;
	    sf::FloatRect bounds(position.x + radius + 10, position.y - height / 2, width, height);
	    graphics::draw_framed_text(w, res, bounds, sf::Color::White, sf::Color(20, 30, 40, 80), fs);
	  }
	}
      }

      // draw shield indicator
      float sp = compute_shield_power();
      if (sp > 0) {
	graphics::draw_circle(w, position, radius + 4, sf::Color(100, 140, 200, 150), sf::Color::Transparent, sp);
      }

      // draw health indicator
      float hp_ratio = compute_hp_ratio();
      if (hp_ratio < 1) {
	sf::FloatRect bounds(position.x - radius, position.y + radius + 5, 2 * radius, 2);
	w.draw(graphics::build_rect(bounds, 0.5, sf::Color::White, sf::Color::Transparent));
	bounds.width *= hp_ratio;
	w.draw(graphics::build_rect(bounds, 0, sf::Color::Transparent, sf::Color::Red));
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
	  bool is_active = choice_data.do_develop() && x.first == choice_data.building_queue.front();
	  if (f.level == 0 && !is_active) continue;
	  
	  res += "\n" + x.first + ": " + to_string(f.level);
	  
	  if (is_active) {
	    f = developed(x.first, 1);
	    res += " (" + to_string((int)(100 * f.progress / f.cost_time)) + "%)";
	  }
	}

	if (choice_data.do_produce()) {
	  res += "\n<<Shipyard>>";
	  if (ship_progress > 0) {
	    string sc = choice_data.ship_queue.front();
	    ship_stats s = ship::table().at(sc);
	    res += "\n" + sc + ": " + to_string((int)(100 * ship_progress / s.build_time)) + "%";
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
      float us = graphics::unscale();
      float flag_scale = 30 * us;
      point flag_point = position + flag_scale * point(0.5, -1.5);
      float dc = utility::l2norm(p - position);
      float df = utility::l2norm(p - flag_point);
      
      d = fmin(df, dc) / us;
      return d < radius;
    }

    template<>
    bool specific_selector<fleet>::is_selectable() {
      return true;
    }

    template<>
    set<combid> specific_selector<fleet>::get_ships(){
      return ships;
    }

    template<>
    void specific_selector<fleet>::draw(window_t &w){
      auto f = [this, &w] (float r, sf::Color cf, sf::Color co, float th = 1) {
	sf::CircleShape s(r);
	s.setPointCount(r / graphics::unscale());
	s.setFillColor(cf);
	s.setOutlineColor(co);
	s.setOutlineThickness(graphics::unscale() * th);
	s.setPosition(position - point(r, r));
	w.draw(s);
      };

      f(radius * graphics::unscale(), graphics::fleet_fill, graphics::fleet_outline);
      f(vision(), sf::Color::Transparent, sf::Color(40, 200, 60, 50));
      if (selected) f(2 * radius * graphics::unscale(), sf::Color::Transparent, sf::Color::White, 3);

      // add a flag
      auto counts = ship_counts();
      list<string> keys = utility::get_map_keys(counts);
      keys.sort([this](string a, string b) {
	  ship_stats ssa = ship::table().at(a);
	  ship_stats ssb = ship::table().at(b);
	  return ssa.get_strength() > ssb.get_strength();
	});

      string ship_key = "scout";
      if (keys.size() > 0) ship_key = keys.front();
      sf::Color bg = is_idle() ? sf::Color::Yellow : sf::Color::White;
      graphics::draw_flag(w, position, get_color(), bg, get_ships().size(), ship_key, (int)log(ships.size() + 1) + 1);

      // add path      
      sf::CircleShape s(4);
      vector<sf::Vertex> line;
      s.setOutlineColor(sf::Color::White);
      s.setOutlineThickness(graphics::unscale());
      s.setFillColor(sf::Color::Transparent);
      list<point> buf = path;
      buf.push_front(heading);
      buf.push_front(position);
      for (auto x : buf) {
	line.push_back(sf::Vertex(x));
	s.setPosition(x - point(4,4));
	w.draw(s);
      }
      w.draw(&line[0], line.size(), sf::LineStrip);

      if (sf::Keyboard::isKeyPressed(sf::Keyboard::L)) {
	// debug: add suggestion
	// hm_t<int, string> suggest_names;
	// suggest_names[suggestion::summon] = "summon";
	// suggest_names[fleet::suggestion::engage] = "engage";
	// suggest_names[fleet::suggestion::scatter] = "scatter";
	// suggest_names[fleet::suggestion::travel] = "travel";
	// suggest_names[fleet::suggestion::activate] = "activate";
	// suggest_names[fleet::suggestion::hold] = "hold";
	// suggest_names[fleet::suggestion::evade] = "evade";

	// graphics::draw_text(w, suggest_names[suggest_buf.id], position, 26);

	// debug: add evade path
	if (stats.can_evade) {
	  line.clear();
	  line.push_back(sf::Vertex(position, sf::Color::Blue));
	  line.push_back(sf::Vertex(stats.evade_path, sf::Color::Blue));
	  w.draw(&line[0], line.size(), sf::LineStrip);
	}
      }
    }

    template<>
    string specific_selector<fleet>::hover_info(){
      string res = "fleet at " + utility::format_float(position.x) + "x" + utility::format_float(position.y);

      if (owned){
	hm_t<string, int> counts = ship_counts();

	res += "\naction: " + com.action;
	for (auto x : counts) res += "\n" + x.first + "s: " + to_string(x.second);
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

      // debug
      graphics::draw_circle(w, position, radius, sf::Color(255,255,255,50));
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
      int load_percent = stats[sskey::key::load_time] > 0 ? 100 * load / stats[sskey::key::load_time] : 100;
      output += build_percent("Hull", get_percent(sskey::key::hp));
      output += build_percent("Weapons", load_percent);
      
      if (base_stats.stats[sskey::key::shield] > 0) {
	output += build_percent("Shields", get_percent(sskey::key::shield));
      }

      auto maybe_include = [this, &output] (string label, int value) {
	if (value > 0) output += label + ": " + to_string(value) + "\n";
      };
      
      if (is_loaded) {
	if (cargo.count()) {
	  maybe_include("Cargo: metals", cargo[keywords::key_metals]);
	  maybe_include("Cargo: gases", cargo[keywords::key_gases]);
	  maybe_include("Cargo: organics", cargo[keywords::key_organics]);
	} else {
	  output += "Cargo: empty\n";
	}
      }

      maybe_include("Passengers", passengers);
      maybe_include("Kills", nkills);

      if (upgrades.size()) {
	output += "Upgrades:\n";
	for (auto u : upgrades) output += u + "\n";
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


hm_t<string, int> entity_selector::ship_counts() {
  hm_t<string, int> counts;
  for (auto sid : get_ships()) counts[g -> get_specific<ship>(sid) -> ship_class]++;
  return counts;
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
  hm_t<int, string> policy_symbols;
  hm_t<int, sf::Color> policy_colors;
  
  policy_symbols[fleet::policy_aggressive] = "A";
  policy_symbols[fleet::policy_evasive] = "E";
  policy_symbols[fleet::policy_maintain_course] = "M";

  policy_colors[fleet::policy_aggressive] = sf::Color::Red;
  policy_colors[fleet::policy_evasive] = sf::Color::Yellow;
  policy_colors[fleet::policy_maintain_course] = sf::Color::Blue;
  
  text.setFont(graphics::default_font); 
  text.setString(policy_symbols[policy] + ": " + to_string(ships.size()));
  text.setCharacterSize(16);
  // text.setStyle(sf::Text::Underlined);
  sf::FloatRect text_dims = text.getLocalBounds();
  text.setPosition(utility::scale_point(to + from, 0.5) - utility::scale_point(point(text_dims.width/2, text_dims.height + 10), graphics::unscale()));
  text.setScale(graphics::inverse_scale(w));
  text.setFillColor(sf::Color::Black);

  // setup arrow colors
  if (selected){
    c_head[0].color = graphics::command_selected_head;
    c_head[1].color = graphics::command_selected_body;
    c_head[2].color = graphics::command_selected_body;
    c_body[0].color = graphics::command_selected_body;
    c_body[1].color = graphics::command_selected_body;
    c_body[2].color = graphics::command_selected_tail;
  }else{
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

  sf::Color text_bg = graphics::fade_color(sf::Color::White, policy_colors[policy], 0.1);
  sf::RectangleShape r = graphics::build_rect(text_dims, 2, policy_colors[policy], text_bg);
  r.setPosition(text.getPosition());
  r.setSize(point(1.5 * text_dims.width, 1.5 * text_dims.height));
  r.setScale(graphics::inverse_scale(w));
  w.draw(r);
  w.draw(text);
}
