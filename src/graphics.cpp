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

  if (s.ship_class == "scout"){
    svert.resize(4);
    svert[0] = sf::Vertex(point(2, 0), col);
    svert[1] = sf::Vertex(point(-2, -1), col);
    svert[2] = sf::Vertex(point(-2, 1), col);
    svert[3] = sf::Vertex(point(2, 0), col);
  }else if (s.ship_class == "fighter"){
    svert.resize(5);
    svert[0] = sf::Vertex(point(2, 0), cnose);
    svert[1] = sf::Vertex(point(-2, -1), col);
    svert[2] = sf::Vertex(point(-3, 0), col);
    svert[3] = sf::Vertex(point(-2, 1), col);
    svert[4] = sf::Vertex(point(2, 0), cnose);
  }else if (s.ship_class == "bomber"){
    svert.resize(7);
    svert[0] = sf::Vertex(point(2, 0), col);
    svert[1] = sf::Vertex(point(0, -3), cnose);
    svert[2] = sf::Vertex(point(-2, -3), cnose);
    svert[3] = sf::Vertex(point(-1, 0), col);
    svert[4] = sf::Vertex(point(-2, 3), cnose);
    svert[5] = sf::Vertex(point(0, 3), cnose);
    svert[6] = sf::Vertex(point(2, 0), col);
  }else if (s.ship_class == "colonizer"){
    svert.resize(5);
    svert[0] = sf::Vertex(point(2, 1), col);
    svert[1] = sf::Vertex(point(2, -1), col);
    svert[2] = sf::Vertex(point(-2, -1), col);
    svert[3] = sf::Vertex(point(-2, 1), col);
    svert[4] = sf::Vertex(point(2, 1), col);
  }else{
    cout << "invalid ship type: " << s.ship_class << endl;
    exit(-1);
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
point graphics::ul_corner(window_t &w){
  sf::View sv = w.getView();
  point v = sv.getSize();
  return sv.getCenter() - point(v.x * 0.5, v.y * 0.5);
}

// transform from pixels to points
sf::Transform graphics::view_inverse_transform(window_t &w){
  sf::Transform t;
  sf::View v = w.getView();

  t.translate(ul_corner(w));
  t.scale(v.getSize().x / w.getSize().x, v.getSize().y / w.getSize().y);
  return t;
}

// scale from pixels to points
point graphics::inverse_scale(window_t &w){
  sf::View v = w.getView();
  return point(v.getSize().x / w.getSize().x, v.getSize().y / w.getSize().y);
}

/* **************************************** */
/* INTERFACES */
/* **************************************** */

using namespace sfg;
using namespace interface;

main_interface *interface::desktop;

// build and wrap in shared ptr

bottom_panel::Ptr bottom_panel::Create(bool &done, bool &accept){
  auto buf = Ptr(new bottom_panel());
  
  auto layout = Box::Create(Box::Orientation::HORIZONTAL);
  auto b_proceed = Button::Create("PROCEED");

  b_proceed -> GetSignal(Widget::OnLeftClick).Connect([&done, &accept](){
      done = true;
      accept = true;
    });

  layout -> Pack(b_proceed);
  buf -> Add(layout);
  return buf;
}

top_panel::Ptr top_panel::Create(){
  auto buf = Ptr(new top_panel());

  auto layout = Box::Create(Box::Orientation::HORIZONTAL);
  auto b_research = Button::Create("RESEARCH");

  b_research -> GetSignal(Widget::OnLeftClick).Connect([](){
      desktop -> reset_qw(research_window::Create(&desktop -> response.research));
    });

  layout -> Pack(b_research);
  buf -> Add(layout);
  return buf;
}

research_window::Ptr research_window::Create(choice::c_research *c){return Ptr(new research_window(c));}

solar_query::main_window::Ptr solar_query::main_window::Create(int id, solar::solar s){
  auto buf = Ptr(new solar_query::main_window(id, s));
  buf -> Add(buf -> layout);
  return buf;
}

solar_query::expansion::Ptr solar_query::expansion::Create(choice::c_expansion &c){
  auto buf = Ptr(new solar_query::expansion(c));
  buf -> Add(buf -> layout);
  return buf;
}

solar_query::military::Ptr solar_query::military::Create(choice::c_military &c){
  auto buf = Ptr(new solar_query::military(c));
  buf -> Add(buf -> layout);
  return buf;
}

solar_query::mining::Ptr solar_query::mining::Create(choice::c_mining &c){
  auto buf = Ptr(new solar_query::mining(c));
  buf -> Add(buf -> layout);
  return buf;
}

// constructors
//query
template<typename C, typename R>
query<C,R>::query() : Window(Window::Style::TOPLEVEL) { }

// boxed
solar_query::boxed::boxed(Box::Orientation o){
  layout = Box::Create(o);
}

const string& solar_query::boxed::GetName() const {
  static string buf = "boxed";
  return buf;
};

sf::Vector2f solar_query::boxed::CalculateRequisition(){
  return layout -> GetRequisition();
}

// main interface

main_interface::main_interface(sf::Vector2u d, research::data &r) : research_level(r), dims(d){
  done = false;
  accept = false;
  
  // build geometric data
  int top_height = 0.1 * d.y;
  int bottom_start = 0.9 * d.y;
  int bottom_height = d.y - bottom_start - 1;
  qw_top = 0.3 * d.y;
  qw_bottom = 0.7 * d.y;
  
  auto top = top_panel::Create();
  top -> SetPosition(sf::Vector2f(0,0));
  top -> SetRequisition(sf::Vector2f(d.x, top_height));
  Add(top);
  
  auto bottom = bottom_panel::Create(done, accept);
  bottom -> SetPosition(sf::Vector2f(0, bottom_start));
  bottom -> SetRequisition(sf::Vector2f(d.x, bottom_height));
  Add(bottom);
}

void main_interface::reset_qw(Widget::Ptr w){
  clear_qw();
  if (w == 0) return;
  
  query_window = w;
  Add(query_window);
}

void main_interface::clear_qw(){
  if (query_window) Remove(query_window);
  query_window = 0;
}

bottom_panel::bottom_panel() {
}

top_panel::top_panel() : Window(Window::Style::TOPLEVEL) {
}

// research window
research_window::research_window(choice::c_research *c) {
  // todo: write me
}


// build a labeled button to modify priority data
Button::Ptr factory (string label, float &data, function<bool()> inc_val){
  auto b = Button::Create(label + to_string(data));

  b -> GetSignal(Widget::OnLeftClick).Connect([&data, b, label, inc_val](){
      if (inc_val()) data++;
      b -> SetLabel(label + ": " + to_string(data));
    });
    
  b -> GetSignal(Widget::OnRightClick).Connect([&data, b, label](){
      if (data > 0) data--;
      b -> SetLabel(label + ": " + to_string(data));
    });

  return b;
};


// main window for solar choice
solar_query::main_window::main_window(idtype solar_id, solar::solar s){
  // sub interface tracker
  sub_window = 0;
  layout = Box::Create(Box::Orientation::HORIZONTAL);
  selection_layout = Box::Create(Box::Orientation::VERTICAL);

  response = desktop -> response.solar_choices[solar_id];

  selection_layout -> Pack(Label::Create("Customize solar choice for solar " + to_string(solar_id)));
  selection_layout -> Pack(Label::Create("Click left/right to add/reduce"));
  selection_layout -> Pack(Separator::Create(Separator::Orientation::HORIZONTAL));

  // sector allocation buttons
  hm_t<string, function<void()> > subcall;

  subcall[cost::keywords::key_mining] = [this](){
    if (sub_window) layout -> Remove(sub_window);
    sub_window = solar_query::mining::Create(response.mining);
    layout -> Pack(sub_window);
  };

  subcall[cost::keywords::key_military] = [this](){
    if (sub_window) layout -> Remove(sub_window);
    sub_window = solar_query::military::Create(response.military);
    layout -> Pack(sub_window);
  };

  subcall[cost::keywords::key_expansion] = [this](){
    if (sub_window) layout -> Remove(sub_window);
    sub_window = solar_query::expansion::Create(response.expansion);
    layout -> Pack(sub_window);
  };

  for (auto v : cost::keywords::sector){
    auto b = factory(v, response.allocation[v], [this](){return response.allocation.count() < choice::max_allocation;});

    if (subcall.count(v)){
      // sectors with sub interfaces
      auto sub = Button::Create(">");
      sub -> GetSignal(Widget::OnLeftClick).Connect(subcall[v]);
      auto l = Box::Create(Box::Orientation::HORIZONTAL);
      l -> Pack(b);
      l -> Pack(sub);
      selection_layout -> Pack(l);
    }else{
      // sectors without sub interfaces
      selection_layout -> Pack(b);
    }
  }

  auto b_accept = Button::Create("ACCEPT");

  b_accept -> GetSignal(Widget::OnLeftClick).Connect([=] () {
      desktop -> response.solar_choices[solar_id] = response;
      desktop -> clear_qw();
    });
  
  auto b_cancel = Button::Create("CANCEL");

  b_cancel -> GetSignal(Widget::OnLeftClick).Connect([] () {
      desktop -> clear_qw();
    });

  auto l_response = Box::Create(Box::Orientation::HORIZONTAL);
  l_response -> Pack(b_accept);
  l_response -> Pack(b_cancel);

  selection_layout -> Pack(l_response);

  layout -> Pack(selection_layout);
}


// sub window for expansion choice
solar_query::expansion::expansion(choice::c_expansion &c) : boxed(Box::Orientation::VERTICAL){
  // add buttons for expandable sectors
  for (auto v : cost::keywords::sector){
    if (v != "expansion"){
      layout -> Pack(factory(v, c[v], [&c](){return c.count() < choice::max_allocation;}));
    }
  }
};

// sub window for military choice
solar_query::military::military(choice::c_military &c) : boxed(Box::Orientation::VERTICAL){
  // add buttons for expandable sectors
  for (auto v : cost::keywords::ship)
    layout -> Pack(factory(v, c.c_ship[v], [&c](){return c.c_ship.count() < choice::max_allocation;}));

  for (auto v : cost::keywords::turret)
    layout -> Pack(factory(v, c.c_turret[v], [&c](){return c.c_turret.count() < choice::max_allocation;}));
};

// sub window for mining choice
solar_query::mining::mining(choice::c_mining &c) : boxed(Box::Orientation::VERTICAL){
  // add buttons for expandable sectors
  for (auto v : cost::keywords::resource)
    layout -> Pack(factory(v, c[v], [&c](){return c.count() < choice::max_allocation;}));
};
