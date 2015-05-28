#include <iostream>

#include "target_gui.h"
#include "utility.h"
#include "command.h"

using namespace std;
using namespace st3;

const target_gui::option_t target_gui::option_cancel("cancel", "");
const target_gui::option_t target_gui::option_add_waypoint("add_waypoint", command::action_waypoint);

const point target_gui::option_size = point(200, 30);

target_gui::option_t::option_t(source_t k, string v) : key(k), option(v){}

st3::target_gui::target_gui(point p, list<target_gui::option_t> opts, sf::RenderWindow *w) : selected_option("",""){
  if (opts.empty()){
    cout << "attempting to build target gui without options!" << endl;
    exit(-1);
  }

  options.insert(options.begin(), opts.begin(), opts.end());
  window = w;
  highlight_index = -1;
  done = false;
  bounds = sf::FloatRect(p.x, p.y, option_size.x, opts.size() * option_size.y);
}

bool st3::target_gui::handle_event(sf::Event e){
  point p;
  int i;

  switch(e.type){
  case sf::Event::MouseButtonReleased:
    p = window -> mapPixelToCoords(sf::Vector2i(e.mouseButton.x, e.mouseButton.y));
    i = compute_index(p);
    if (i > -1){
      selected_option = options[i];
      done = true;
      return true;
    }
    break;
  case sf::Event::MouseButtonPressed:
    p = window -> mapPixelToCoords(sf::Vector2i(e.mouseButton.x, e.mouseButton.y));
    return bounds.contains(p);
  case sf::Event::MouseMoved:
    p = window -> mapPixelToCoords(sf::Vector2i(e.mouseMove.x, e.mouseMove.y));

    highlight_index = compute_index(p);

    cout << "target gui: mouse move: " << p << ", root = " << point(bounds.left, bounds.top) << ", index = " << highlight_index << endl;
    break;
  case sf::Event::KeyPressed:
    switch(e.key.code){
    case sf::Keyboard::Return:
      selected_option = options[0];
      done = true;
      return true;
    }
  }

  return false;
}

int st3::target_gui::compute_index(point p){
  if (bounds.contains(p)){
    return (p.y - bounds.top) / option_size.y;
  }else{
    return -1;
  }
}

void st3::target_gui::draw(){
  sf::Text text;
  sf::RectangleShape r;

  text.setFont(graphics::default_font); 
  text.setCharacterSize(0.8 * option_size.y);
  text.setColor(sf::Color(10,20,30));

  r.setSize(option_size);
  r.setOutlineColor(sf::Color(200,200,200));
  r.setOutlineThickness(2);
  
  for (int i = 0; i < options.size(); i++){
    r.setPosition(bounds.left, bounds.top + i * option_size.y);
    if (i == highlight_index){
      r.setFillColor(sf::Color(150,150,150));
    }else{
      r.setFillColor(sf::Color(150,150,150,150));
    }

    text.setString(options[i].key + " - " + options[i].option);
    text.setPosition(bounds.left, bounds.top + i * option_size.y);
    
    window -> draw(r);
    window -> draw(text);
  }
}
