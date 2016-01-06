#include <cmath>
#include <iostream>

#include "graphics.h"
#include "types.h"

using namespace std;
using namespace st3;
using namespace graphics;

sf::Font graphics::default_font;

sf::Color graphics::sfcolor(sint c){
  sint mask = 0xff;
  return sf::Color(mask & (c >> 16), mask & (c >> 8), mask & c, mask & (c >> 24));
}

sf::Color graphics::fade_color(sf::Color from, sf::Color to, float r){
  return sf::Color(from.r + r * (to.r - from.r), 
		   from.g + r * (to.g - from.g), 
		   from.b + r * (to.b - from.b), 
		   from.a + r * (to.a - from.a));
}

void graphics::initialize(){
  
  // setup load text
  if (!default_font.loadFromFile("fonts/AjarSans-Regular.ttf")){
    cout << "error loading font" << endl;
    exit(-1);
  }
}

void graphics::draw_ship(window_t &w, ship s, sf::Color col, float sc){
  vector<sf::Vertex> svert;

  sf::Color cnose(255,200,180,200);

  switch (s.ship_class){
  case solar::ship_scout:
    svert.resize(4);
    svert[0] = sf::Vertex(point(2, 0), col);
    svert[1] = sf::Vertex(point(-2, -1), col);
    svert[2] = sf::Vertex(point(-2, 1), col);
    svert[3] = sf::Vertex(point(2, 0), col);
    break;
  case solar::ship_fighter:
    svert.resize(5);
    svert[0] = sf::Vertex(point(2, 0), cnose);
    svert[1] = sf::Vertex(point(-2, -1), col);
    svert[2] = sf::Vertex(point(-3, 0), col);
    svert[3] = sf::Vertex(point(-2, 1), col);
    svert[4] = sf::Vertex(point(2, 0), cnose);
    break;
  case solar::ship_bomber:
    svert.resize(7);
    svert[0] = sf::Vertex(point(2, 0), col);
    svert[1] = sf::Vertex(point(0, -3), cnose);
    svert[2] = sf::Vertex(point(-2, -3), cnose);
    svert[3] = sf::Vertex(point(-1, 0), col);
    svert[4] = sf::Vertex(point(-2, 3), cnose);
    svert[5] = sf::Vertex(point(0, 3), cnose);
    svert[6] = sf::Vertex(point(2, 0), col);
    break;
  case solar::ship_colonizer:
    svert.resize(5);
    svert[0] = sf::Vertex(point(2, 1), col);
    svert[1] = sf::Vertex(point(2, -1), col);
    svert[2] = sf::Vertex(point(-2, -1), col);
    svert[3] = sf::Vertex(point(-2, 1), col);
    svert[4] = sf::Vertex(point(2, 1), col);
    break;
  default:
    cout << "invalid ship type: " << s.ship_class << endl;
    exit(-1);
  }

  sf::Transform t;
  t.translate(s.position.x, s.position.y);
  t.rotate(s.angle / (2 * M_PI) * 360);
  t.scale(sc, sc);
  w.draw(&svert[0], svert.size(), sf::LinesStrip, t);
}

/* **************************************** */
/* INTERFACES */
/* **************************************** */

using namespace sfg;
using namespace interface;

// build and wrap in shared ptr


bottom_panel::Create(){return Ptr(new bottom_panel());}
top_panel::Create(){return Ptr(new top_panel());}

research_window::Create(){return Ptr(new research_window());}

solar_query::main_window::Create(int id, solar::solar s){return Ptr(new solar_query::main_window(id, s));}
solar_query::expansion::Create(){return Ptr(new solar_query::military());}
solar_query::military::Create(){return Ptr(new solar_query::military());}
solar_query::mining::Create(){return Ptr(new solar_query::mining());}

// constructors

// main interface

main_interface::main_interface(sf::Vector2u d, research::data r) : research_level(r), dims(d){
  // build geometric data
  int top_height = 0.2 * d.y;
  int bottom_start = 0.8 * d.y;
  int bottom_height = d.y - bottom_start - 1;
  qw_top = 0.3 * d.y;
  qw_bottom = 0.7 * d.y;
  
  auto top = top_panel::Create();
  top -> SetPosition(sf::Vector2i(0,0));
  top -> SetSize(sf::Vector2u(d.x, top_height));
  Add(top);
  
  auto bottom = bottom_panel::Create();
  bottom -> SetPosition(sf::Vector2i(0, bottom_start));
  bottom -> SetSize(sf::Vector2u(d.x, bottom_height));
  Add(bottom);
}

bottom_panel::bottom_panel(){
  auto layout = Box::Create(Box::Orientation::HORIZONTAL);
  auto b_proceed = Button::Create("PROCEED");

  b_proceed -> GetSignal(Widget::OnLeftClick).Connect([&done, &accept](){
      done = true;
      accept = true;
    };);

  layout -> Pack(b_proceed);
  Add(layout);
}

top_panel::top_panel() {
  auto layout = Box::Create(Box::Orientation::HORIZONTAL);
  auto b_research = Button::Create("RESEARCH");

  b_research -> GetSignal(Widget::OnLeftClick).Connect([](){
      desktop -> reset_query_window(research_window::Create(desktop -> response.research));
    };);

  layout -> Pack(b_research);
  Add(layout);
}

// research window
research_window::research_window(choice::c_research c) : response(c) {
  // todo: write me
}

// main window for solar choice
solar_query::main_window::main_window(solar::choice::choice_t c) : response(c){
  int max_allocation;
  sub_window = 0;
  
  layout = Box::Create(Box::Orientation::VERTICAL);

  layout -> Pack(Label::Create("Customize solar choice for solar " + to_string(solar_id)));
  layout -> Pack(Label::Create("Click left/right to add/reduce"));
  layout -> Pack(Separator::Create(Separator::Orientation::HORIZONTAL));

  // sector allocation buttons
  auto increment_builder = [&response, max_allocation] (string label, int &allocation) {
    auto b = Button::Create(label + ": " + to_string(allocation));
    
    b -> GetSignal(Widget::OnLeftClick).Connect([&response, max_allocation, &allocation](){
	if (response.count_allocation() < max_allocation) allocation++;
      };);

    b -> GetSignal(Widget::OnRightClick).Connect([&response, &allocation](){
	if (allocation > 0) allocation--;
      };);

    return b;
  };
  
  auto b_culture = increment_builder("CULTURE", response.allocation.culture);
  layout -> Pack(b_culture);
  
  auto b_mining = increment_builder("MINING", response.allocation.mining);
  auto b_military = increment_builder("MILITARY", response.allocation.military);
  auto b_expansion = increment_builder("EXPANSION", response.allocation.expansion);

  auto sub_mining = Button::Create(">");
  
  sub_mining -> GetSignal(Widget::OnLeftClick).Connect([&response, sub_window, layout](){
      if (sub_window) layout -> Remove(sub_window);
      sub_window = solar_query::mining::Create(&response.mining);
      layout -> Pack(sub_window);
    };);

  auto sub_military = Button::Create(">");
  
  sub_military -> GetSignal(Widget::OnLeftClick).Connect([&response, sub_window, layout](){
      if (sub_window) layout -> Remove(sub_window);
      sub_window = solar_query::military::Create(&response.military);
      layout -> Pack(sub_window);
    };);

  auto sub_expansion = Button::Create(">");
  
  sub_expansion -> GetSignal(Widget::OnLeftClick).Connect([&response, sub_window, layout](){
      if (sub_window) layout -> Remove(sub_window);
      sub_window = solar_query::expansion::Create(&response.expansion);
      layout -> Pack(sub_window);
    };);

  auto l_mining = Box::Create(Box::Orientation::HORIZONTAL);
  auto l_military = Box::Create(Box::Orientation::HORIZONTAL);
  auto l_expansion = Box::Create(Box::Orientation::HORIZONTAL);

  l_mining -> Pack(b_mining);
  l_mining -> Pack(sub_mining);

  l_military -> Pack(b_military);
  l_military -> Pack(sub_military);

  l_expansion -> Pack(b_expansion);
  l_expansion -> Pack(sub_expansion);

  layout -> Pack(l_mining);
  layout -> Pack(l_military);
  layout -> Pack(l_expansion);

  auto b_accept = Button::Create("ACCEPT");

  b_accept -> GetSignal(Widget::OnLeftClick).Connect([&response] () {
      desktop -> response.solar_choices[solar_id] = response;
      desktop -> clear_qw();
    };);
  
  auto b_cancel = Button::Create("CANCEL");

  b_cancel -> GetSignal(Widget::OnLeftClick).Connect([] () {
      desktop -> clear_qw();
    };);

  auto l_respons = Box::Create(Box::Orientation::HORIZONTAL);
  l_response -> Pack(b_accept);
  l_response -> Pack(b_cancel);

  layout -> Pack(l_response);

  Add(layout);
}

// sub window for expansion choice
solar_query::expansion::expansion(solar::choice::c_expansion c) : response(c){

};

// sub window for military choice
solar_query::military::military(solar::choice::c_military c) : response(c){

};

// sub window for mining choice
solar_query::mining::s_mining(solar::choice::c_mining c) : response(c){

};
