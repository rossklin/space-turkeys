#include <cmath>
#include <iostream>

#include "graphics.h"
#include "types.h"

using namespace std;
using namespace st3;
using namespace graphics;

sf::Font graphics::default_font;
sf::Vector2u interface::desktop_dims;
sf::FloatRect interface::qw_allocation;
int interface::top_height;
int interface::bottom_start;

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

  buf -> SetAllocation(sf::FloatRect(0, bottom_start, desktop_dims.x, desktop_dims.y - bottom_start));
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
  buf -> SetAllocation(sf::FloatRect(0, 0, desktop_dims.x, top_height));
  return buf;
}

research_window::Ptr research_window::Create(choice::c_research *c){return Ptr(new research_window(c));}

solar_query::main_window::Ptr solar_query::main_window::Create(int id, solar::solar s){
  auto buf = Ptr(new solar_query::main_window(id, s));
  buf -> Add(buf -> layout);
  buf -> SetAllocation(qw_allocation);
  buf -> SetTitle("Customize solar choice for solar " + to_string(buf -> solar_id));

  return buf;
}

solar_query::expansion::Ptr solar_query::expansion::Create(choice::c_expansion &c){
  auto buf = Ptr(new solar_query::expansion(c));
  buf -> Add(buf -> layout);
  buf -> SetRequisition(desktop -> sub_dims());
  return buf;
}

solar_query::military::Ptr solar_query::military::Create(choice::c_military &c){
  auto buf = Ptr(new solar_query::military(c));
  buf -> Add(buf -> layout);
  buf -> SetRequisition(desktop -> sub_dims());
  return buf;
}

solar_query::mining::Ptr solar_query::mining::Create(choice::c_mining &c){
  auto buf = Ptr(new solar_query::mining(c));
  buf -> Add(buf -> layout);
  buf -> SetRequisition(desktop -> sub_dims());
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

main_interface::main_interface(sf::Vector2u d, research::data &r) : research_level(r){
  done = false;
  accept = false;
  desktop_dims = d;
  
  // build geometric data
  top_height = 0.1 * d.y;
  bottom_start = 0.9 * d.y;
  int bottom_height = d.y - bottom_start - 1;
  int qw_top = 0.2 * d.y;
  int qw_bottom = 0.8 * d.y;
  qw_allocation = sf::FloatRect(10, qw_top, d.x - 20, qw_bottom - qw_top);
  
  auto top = top_panel::Create();
  Add(top);
  
  auto bottom = bottom_panel::Create(done, accept);
  Add(bottom);
}

sf::Vector2f main_interface::sub_dims(){
  return sf::Vector2f(qw_allocation.width/2, qw_allocation.height);
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

string label_builder(string label, float data){
  return label + ": " + to_string((int)round(data));
}

// build a labeled button to modify priority data
Button::Ptr factory (string label, float &data, function<bool()> inc_val, Label::Ptr tip = 0){
  auto b = Button::Create(label_builder(label, data));
  auto p = b.get();

  b -> GetSignal(Widget::OnLeftClick).Connect([&data, p, label, inc_val](){
      if (inc_val()) data++;
      p -> SetLabel(label_builder(label, data));
    });
    
  b -> GetSignal(Widget::OnRightClick).Connect([&data, p, label](){
      if (data > 0) data--;
      p -> SetLabel(label_builder(label, data));
    });

  if (tip){
    b -> GetSignal(Widget::OnMouseEnter).Connect([tip, label](){
	tip -> SetText("Modify priority for " + label);
      });
  }

  return b;
};


// main window for solar choice
solar_query::main_window::main_window(idtype sid, solar::solar s) : sol(s), solar_id(sid){
  response = desktop -> response.solar_choices[solar_id];

  // main layout
  layout = Box::Create(Box::Orientation::VERTICAL, 20);

  layout -> Pack(Separator::Create(Separator::Orientation::HORIZONTAL));
  tooltip = Label::Create("...");
  layout -> Pack(tooltip);
  layout -> Pack(Separator::Create(Separator::Orientation::HORIZONTAL));

  auto meta_layout = Box::Create(Box::Orientation::HORIZONTAL);
  meta_layout -> Pack(choice_layout = Box::Create(Box::Orientation::VERTICAL));
  meta_layout -> Pack(Separator::Create(Separator::Orientation::VERTICAL));
  meta_layout -> Pack(sub_layout = Box::Create(Box::Orientation::VERTICAL));
  meta_layout -> Pack(Separator::Create(Separator::Orientation::VERTICAL));
  meta_layout -> Pack(info_layout = Box::Create(Box::Orientation::VERTICAL));
    
  // choice template buttons
  auto template_map = choice::c_solar::template_table();
  hm_t<string, Button::Ptr> alloc_button;
  auto template_layout = Box::Create(Box::Orientation::HORIZONTAL);

  for (auto &x : template_map){
    auto b = Button::Create("Template: " + x.first);
    b -> GetSignal(Widget::OnLeftClick).Connect([this, x] () {
	response = x.second;
	build_choice();
      });
    template_layout -> Pack(b);
  }

  layout -> Pack(template_layout);
  layout -> Pack(meta_layout);

  build_choice();
  build_info();

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

  layout -> Pack(l_response);
  layout -> SetRequisition(desktop -> sub_dims());
}

void solar_query::main_window::build_choice(){
  // sector allocation buttons
  auto layout = Box::Create(Box::Orientation::VERTICAL);
  hm_t<string, function<Widget::Ptr()> > subq;
  subq[cost::keywords::key_mining] = [this] () {return solar_query::mining::Create(response.mining);};
  subq[cost::keywords::key_military] = [this] () {return solar_query::military::Create(response.military);};
  subq[cost::keywords::key_expansion] = [this] () {return solar_query::expansion::Create(response.expansion);};

  for (auto v : cost::keywords::sector){
    auto b = factory(v, response.allocation[v], [this](){return response.allocation.count() < choice::max_allocation;}, tooltip);

    if (subq.count(v)){
      // sectors with sub interfaces
      auto sub = Button::Create(">");
      sub -> GetSignal(Widget::OnLeftClick).Connect([this,v,subq](){build_sub(subq.at(v)());});
      auto l = Box::Create(Box::Orientation::HORIZONTAL);
      l -> Pack(b);
      l -> Pack(sub);
      layout -> Pack(l);
    }else{
      // sectors without sub interfaces
      layout -> Pack(b);
    }
  }

  auto frame = Frame::Create("Sector priorities");
  frame -> Add(layout);

  choice_layout -> RemoveAll();
  choice_layout -> Pack(frame);
}

void solar_query::main_window::build_sub(Widget::Ptr p){
  sub_layout -> RemoveAll();
  sub_layout -> Pack(p);
}

void solar_query::main_window::build_info(){
  auto res = Box::Create(Box::Orientation::VERTICAL);
  res -> Pack(Label::Create("Population: " + to_string((int)sol.population)));
  res -> Pack(Label::Create("Happiness: " + to_string(sol.happiness)));
  res -> Pack(Label::Create("Ecology: " + to_string(sol.ecology)));
  res -> Pack(Label::Create("Water: " + to_string(sol.water_status())));
  res -> Pack(Label::Create("Space: " + to_string(sol.space_status())));

  for (auto v : cost::keywords::sector)
    res -> Pack(Label::Create("Sector " + v + ": " + to_string(sol.sector[v])));

  for (auto v : cost::keywords::resource)
    res -> Pack(Label::Create("Resource " + v + ": " + to_string(sol.resource[v].storage) + "[" + to_string(sol.resource[v].available) + "]"));

  res -> Pack(Label::Create("Turrets: " + to_string(sol.turrets.size())));

  auto frame = Frame::Create("Solar info");
  frame -> Add(res);

  info_layout -> RemoveAll();
  info_layout -> Pack(frame);
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
