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

main_window::Ptr main_window::Create(int id, solar::solar s){
  auto buf = Ptr(new main_window(id, s));
  buf -> Add(buf -> layout);
  buf -> SetAllocation(qw_allocation);
  buf -> SetId(string(sfg_id));

  return buf;
}

// constructors
//query
template<typename C, typename R>
query<C,R>::query(char s) : Window(s) { }

// main interface

main_interface::main_interface(sf::Vector2u d, research::data &r) : research_level(r){
  done = false;
  accept = false;
  desktop_dims = d;
  
  // build geometric data
  top_height = 0.1 * d.y;
  bottom_start = 0.9 * d.y;
  int bottom_height = d.y - bottom_start - 1;
  int qw_top = 0.1 * d.y + 10;
  int qw_bottom = 0.8 * d.y;
  qw_allocation = sf::FloatRect(10, qw_top, d.x - 20, qw_bottom - qw_top);
  
  auto top = top_panel::Create();
  Add(top);
  
  auto bottom = bottom_panel::Create(done, accept);
  Add(bottom);

  // set display properties
  SetProperty("Window#" + string(main_window::sfg_id), "BackgroundColor", sf::Color(20, 30, 120, 100));
  SetProperty("Button", "BackgroundColor", sf::Color(30, 180, 120, 140));
  SetProperty("Button", "BorderColor", sf::Color(80, 180, 120, 200));

  SetProperty("Widget", "Color", sf::Color(200, 170, 120));
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

bottom_panel::bottom_panel() : Window(Window::Style::BACKGROUND) {
}

top_panel::top_panel() : Window(Window::Style::BACKGROUND) {
}

// research window
research_window::research_window(choice::c_research *c) {
  // todo: write me
}

// build a labeled button to modify priority data
Button::Ptr main_window::priority_button(string label, float &data, function<bool()> inc_val, Label::Ptr tip){

  auto label_builder = [] (string label, float data){
    return label + ": " + to_string((int)round(data));
  };

  auto b = Button::Create(label_builder(label, data));
  auto p = b.get();

  b -> GetSignal(Widget::OnLeftClick).Connect([this, &data, p, label, inc_val, label_builder](){
      if (inc_val()) data++;
      p -> SetLabel(label_builder(label, data));
      build_info();
    });
    
  b -> GetSignal(Widget::OnRightClick).Connect([this, &data, p, label, label_builder](){
      if (data > 0) data--;
      p -> SetLabel(label_builder(label, data));
      build_info();
    });

  if (tip){
    b -> GetSignal(Widget::OnMouseEnter).Connect([tip, label](){
	tip -> SetText("Modify priority for " + label);
      });
  }

  return b;
};

// main window for solar choice
main_window::main_window(idtype sid, solar::solar s) : query<Window, choice::c_solar>(Window::Style::BACKGROUND), sol(s), solar_id(sid){
  
  response = desktop -> response.solar_choices[solar_id];

  // main layout
  layout = Box::Create(Box::Orientation::VERTICAL, 10);

  layout -> Pack(Separator::Create(Separator::Orientation::HORIZONTAL));
  tooltip = Label::Create("Customize solar choice for solar " + to_string(solar_id));
  layout -> Pack(tooltip);
  layout -> Pack(Separator::Create(Separator::Orientation::HORIZONTAL));

  auto meta_layout = Box::Create(Box::Orientation::HORIZONTAL, 10);
  meta_layout -> Pack(choice_layout = Box::Create(Box::Orientation::VERTICAL));
  meta_layout -> Pack(sub_layout = Box::Create(Box::Orientation::VERTICAL));
  meta_layout -> Pack(info_layout = Box::Create(Box::Orientation::VERTICAL));
    
  // choice template buttons
  auto template_map = choice::c_solar::template_table();
  auto template_layout = Box::Create(Box::Orientation::HORIZONTAL);

  for (auto &x : template_map){
    auto b = Button::Create("Template: " + x.first);
    b -> GetSignal(Widget::OnLeftClick).Connect([this, x] () {
	response = x.second;
	build_choice();
	build_info();
	new_sub("(chose sub edit)");
      });
    b -> GetSignal(Widget::OnMouseEnter).Connect([this,x] () {
	tooltip -> SetText("Use template " + x.first);
      });
    template_layout -> Pack(b);
  }

  layout -> Pack(template_layout);
  layout -> Pack(meta_layout);

  build_choice();
  new_sub("(chose sub edit)");
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
}

void main_window::build_choice(){
  // sector allocation buttons
  auto layout = Box::Create(Box::Orientation::VERTICAL);
  hm_t<string, function<void()> > subq;
  subq[cost::keywords::key_mining] = [this] () {build_mining();};
  subq[cost::keywords::key_military] = [this] () {build_military();};
  subq[cost::keywords::key_expansion] = [this] () {build_expansion();};

  for (auto v : cost::keywords::sector){
    auto b = priority_button(v, response.allocation[v], [this](){return response.allocation.count() < choice::max_allocation;}, tooltip);

    auto l = Box::Create(Box::Orientation::HORIZONTAL);
    l -> Pack(b);

    if (subq.count(v)){
      // sectors with sub interfaces
      auto sub = Button::Create(">");
      sub -> GetSignal(Widget::OnLeftClick).Connect([v,subq] () {subq.at(v)();});
      sub -> GetSignal(Widget::OnMouseEnter).Connect([this,v] () {tooltip -> SetText("Edit sub choices for " + v);});
      l -> Pack(sub);
    }
    
    layout -> Pack(l);
  }

  auto frame = Frame::Create("Sector priorities");
  frame -> Add(layout);

  choice_layout -> RemoveAll();
  choice_layout -> Pack(frame);
}

string format_float(float x){
  stringstream stream;
  stream << fixed << setprecision(2) << x;
  return stream.str();
}

void main_window::build_info(){
  auto res = Box::Create(Box::Orientation::VERTICAL);
  choice::c_solar c = response;
  int width = 50;
  c.normalize();
  
  auto patch_label = [this, width] (Label::Ptr a, string v){
    a -> SetAlignment(sf::Vector2f(0, 0));
    a -> GetSignal(Widget::OnMouseEnter).Connect([this, v] () {
	tooltip -> SetText("Status for " + v + " (rate of change in parenthesis)");
      });
  };    

  auto label_build = [this, patch_label] (string v, float absolute, float delta) -> Label::Ptr{
    auto a = Label::Create(v + ": " + format_float(absolute) + " [" + format_float(delta) + "]");
    patch_label(a, v);
    return a;
  };

  auto resource_label_build = [this, patch_label] (string v, float absolute, float delta, float avail) -> Label::Ptr{
    auto a = Label::Create(v + ": " + format_float(absolute) + " [" + format_float(delta) + "] - " + format_float(avail));
    patch_label(a, v);
    return a;
  };

  auto frame = [width] (string title, sfg::Widget::Ptr content) {
    // auto frame_buf = Frame::Create(title);
    // frame_buf -> Add(content);
    // return frame_buf;
    auto buf = Box::Create(Box::Orientation::VERTICAL);
    buf -> Pack(Separator::Create(Separator::Orientation::HORIZONTAL));
    buf -> Pack(Label::Create(title));
    buf -> Pack(Separator::Create(Separator::Orientation::HORIZONTAL));
    buf -> Pack(content);
    return buf;
  };

  auto buf = Box::Create(Box::Orientation::VERTICAL);
  buf -> Pack(label_build("Population", sol.population, sol.poluation_increment()));
  buf -> Pack(label_build("Happiness", sol.happiness, sol.happiness_increment(c)));
  buf -> Pack(label_build("Ecology", sol.ecology, sol.ecology_increment()));
  buf -> Pack(label_build("Water", sol.water_status(), 0));
  buf -> Pack(label_build("Space", sol.space_status(), 0));

  res -> Pack(frame("Stats", buf));

  buf = Box::Create(Box::Orientation::VERTICAL);
  for (auto v : cost::keywords::expansion)
    buf -> Pack(label_build(v, sol.sector[v], sol.expansion_increment(v, c)));

  res -> Pack(frame("Sectors", buf));

  buf = Box::Create(Box::Orientation::VERTICAL);
  for (auto v : cost::keywords::resource)
    buf -> Pack(resource_label_build(v, sol.resource[v].storage, sol.resource_increment(v, c), sol.resource[v].available));

  res -> Pack(frame("Resources", buf));

  buf = Box::Create(Box::Orientation::VERTICAL);
  buf -> Pack(label_build("Ships", sol.ships.size(), 0));
  buf -> Pack(label_build("Turrets", sol.turrets.size(), 0));

  res -> Pack(frame("Military", buf));
  
  info_layout -> RemoveAll();
  info_layout -> Pack(frame("Solar " + to_string(solar_id) + " info", res), false, false);
}

Box::Ptr main_window::new_sub(string v){
  auto buf = Box::Create(Box::Orientation::VERTICAL);
  auto frame = Frame::Create(v);
  frame -> Add(buf);
  sub_layout -> RemoveAll();
  sub_layout -> Pack(frame);
  return buf;
}
 
// sub window for expansion choice
void main_window::build_expansion(){
  choice::c_expansion &c = response.expansion;
  auto buf = new_sub("Expansion priorities");
  
  // add buttons for expandable sectors
  for (auto v : cost::keywords::expansion){
    buf -> Pack(priority_button(v, c[v], [&c](){return c.count() < choice::max_allocation;}, tooltip));
  }
};

// sub window for military choice
void main_window::build_military(){
  choice::c_military &c = response.military;
  auto buf = new_sub("Military build priorities");
  
  // add buttons for expandable sectors
  for (auto v : cost::keywords::ship)
    buf -> Pack(priority_button(v, c.c_ship[v], [&c](){return c.c_ship.count() < choice::max_allocation;}, tooltip));

  for (auto v : cost::keywords::turret)
    buf -> Pack(priority_button(v, c.c_turret[v], [&c](){return c.c_turret.count() < choice::max_allocation;}, tooltip));
};

// sub window for mining choice
void main_window::build_mining(){
  choice::c_mining &c = response.mining;
  auto buf = new_sub("Mining resource priorities");

  // add buttons for expandable sectors
  for (auto v : cost::keywords::resource){
    auto b = priority_button(v, c[v], [&c](){return c.count() < choice::max_allocation;}, tooltip);
    buf -> Pack(b);
  }
};
