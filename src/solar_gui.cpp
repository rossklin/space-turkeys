#include <boost/algorithm/string/join.hpp>

#include "development_tree.h"
#include "development_gui.h"
#include "solar_gui.h"
#include "desktop.h"
#include "research.h"
#include "utility.h"
#include "graphics.h"

using namespace std;
using namespace st3;
using namespace sfg;
using namespace interface;

const std::string solar_gui::sfg_id = "solar gui";

const std::string solar_gui::tab_sectors = "Sectors";
const std::string solar_gui::tab_development = "Development";
const std::string solar_gui::tab_military = "Military";

solar_gui::Ptr solar_gui::Create(solar::ptr s){
  Ptr self(new solar_gui(s));
  self -> Add(self -> layout);
  self -> SetAllocation(main_interface::qw_allocation);
  self -> SetId(sfg_id);
  return self;
}

// main window for solar choice
solar_gui::solar_gui(solar::ptr s) : Window(Window::Style::BACKGROUND), sol(s) {
  response = s -> choice_data;

  // main layout
  layout = Box::Create(Box::Orientation::VERTICAL, 10);

  // header
  layout -> Pack(Separator::Create(Separator::Orientation::HORIZONTAL));
  tooltip = Label::Create("Customize solar choice for solar " + sol -> id);
  layout -> Pack(tooltip);
  layout -> Pack(Separator::Create(Separator::Orientation::HORIZONTAL));

  // info layout
  layout -> Pack(info_layout = Box::Create(Box::Orientation::HORIZONTAL, 10));
  build_info();
  layout -> Pack(Separator::Create(Separator::Orientation::HORIZONTAL));

  // tabs
  Box::Ptr tab_layout = Box::Create(Box::Orientation::HORIZONTAL, 10);
  list<string> tab_names = {tab_sectors, tab_development, tab_military};
  hm_t<string, Button::Ptr> tab_buttons;

  for (auto v : tab_names) {
    Button::Ptr b = Button::Create(v);
    desktop -> bind_ppc(b, [this, v] () {setup(v);});
    tab_layout -> Pack(b);
  }

  layout -> Pack(tab_layout);

  // sub layout for sector, development or military
  sub_layout = Box::Create(Box::Orientation::VERTICAL);
  layout -> Pack(sub_layout);

  auto b_accept = Button::Create("ACCEPT");
  desktop -> bind_ppc(b_accept, [this] () {
      sol -> choice_data = response;
      cout << "Added solar choice for " << sol -> id << endl;
      desktop -> clear_qw();
    });
  
  auto b_cancel = Button::Create("CANCEL");
  desktop -> bind_ppc(b_cancel, [] () {
      desktop -> clear_qw();
    });

  auto l_response = Box::Create(Box::Orientation::HORIZONTAL);
  l_response -> Pack(b_accept);
  l_response -> Pack(b_cancel);

  layout -> Pack(l_response);
}

void solar_gui::setup(string key) {
  sub_layout -> RemoveAll();

  if (key == tab_sectors) {
    setup_sectors();
  } else if (key == tab_development) {
    setup_development();
  } else if (key == tab_military) {
    setup_military();
  } else {
    throw runtime_error("Invalid solar gui setup key: " + key);
  }
}

void solar_gui::setup_development(){
  if (sol -> available_facilities(desktop -> get_research()).empty()) {
    return;
  }
      
  development_gui::f_req_t f_req = [this] (string v) -> list<string> {
    return sol -> list_facility_requirements(v, desktop -> get_research());
  };

  development_gui::f_select_t on_select = [this] (string r) {
    response.development = r;
  };

  hm_t<string, development::node> map;
  for (auto &f : solar::facility_table()) map[f.first] = f.second;
  sub_layout -> Pack(development_gui::Create(map, response.development, f_req, on_select, true));
}

void solar_gui::setup_sectors() {
  auto build_label = [this] (string v) -> Image::Ptr {
    return Image::Create(graphics::selector_card(v, response.allocation[v]));
  };
  
  for (auto v : keywords::sector){
    sector_buttons[v] = Button::Create(v);
    sector_buttons[v] -> SetImage(build_label(v));
    
    desktop -> bind_ppc(sector_buttons[v], [this, v, build_label] () {
	response.allocation[v] = !response.allocation[v];
	sector_buttons[v] -> SetImage(build_label(v));
      });
    
    sub_layout -> Pack(sector_buttons[v]);
  }
}
// sub window for military choice
void solar_gui::setup_military(){
  list<string> req;

  auto build_label = [this] (string v, list<string> requirements = {}) -> Image::Ptr {
    return Image::Create(graphics::selector_card(v, response.military[v], {}, requirements));
  };
  
  // add buttons for ships
  for (auto v : ship::all_classes()) {
    military_buttons[v] = Button::Create(v);
    
    if (desktop -> get_research().can_build_ship(v, sol, &req)) {
      military_buttons[v] -> SetImage(build_label(v));
      desktop -> bind_ppc(military_buttons[v], [this, build_label, v] () {
	  response.military[v] = !response.military[v];
	  military_buttons[v] -> SetImage(build_label(v));
	});
    }else{
      military_buttons[v] -> SetImage(build_label(v, req));
    }

    sub_layout -> Pack(military_buttons[v]);
  }
};

void solar_gui::build_info(){
  auto res = Box::Create(Box::Orientation::VERTICAL);
  choice::c_solar c = response;
  c.normalize();
  
  auto patch_label = [this] (Label::Ptr a, string v){
    a -> SetAlignment(sf::Vector2f(0, 0));
    a -> GetSignal(Widget::OnMouseEnter).Connect([this, v] () {
	tooltip -> SetText("Status for " + v + " [rate of change in brackets]");
      });
  };

  auto label_build = [this, patch_label] (string v, float absolute, float delta) -> Label::Ptr{
    auto a = Label::Create(v + ": " + utility::format_float(absolute) + " [" + utility::format_float(delta) + "]");
    patch_label(a, v);
    return a;
  };

  auto resource_label_build = [this, patch_label] (string v, float absolute, float delta, float avail) -> Label::Ptr{
    auto a = Label::Create(v + ": " + utility::format_float(absolute) + " [" + utility::format_float(delta) + "] - " + utility::format_float(avail));
    patch_label(a, v);
    return a;
  };

  auto frame = [] (string title, sfg::Widget::Ptr content) {
    auto buf = Box::Create(Box::Orientation::VERTICAL);
    buf -> Pack(Separator::Create(Separator::Orientation::HORIZONTAL));
    buf -> Pack(Label::Create(title));
    buf -> Pack(Separator::Create(Separator::Orientation::HORIZONTAL));
    buf -> Pack(content);
    return buf;
  };

  auto buf = Box::Create(Box::Orientation::VERTICAL);
  buf -> Pack(label_build("Population", sol -> population, sol -> population_increment()));
  buf -> Pack(label_build("Happiness", sol -> happiness, sol -> happiness_increment(c)));
  buf -> Pack(label_build("Ecology", sol -> ecology, sol -> ecology_increment()));
  buf -> Pack(label_build("Water", sol -> water_status(), 0));
  buf -> Pack(label_build("Space", sol -> space_status(), 0));
  buf -> Pack(label_build("Research rate", sol -> research_increment(c), 0));
  buf -> Pack(label_build("Development points", sol -> development_points, sol -> development_increment(c)));

  res -> Pack(frame("Stats", buf));

  buf = Box::Create(Box::Orientation::VERTICAL);
  for (auto &f : sol -> development) {
    buf -> Pack(label_build(f.first, f.second.level, sol -> development_increment(c)));
  }

  res -> Pack(frame("Sectors", buf));

  buf = Box::Create(Box::Orientation::VERTICAL);
  for (auto v : keywords::resource) {
    buf -> Pack(resource_label_build(v, sol -> resource_storage[v], sol -> resource_increment(v, c), sol -> available_resource[v]));
  }

  res -> Pack(frame("Resources", buf));

  buf = Box::Create(Box::Orientation::VERTICAL);
  buf -> Pack(label_build("Ships", sol -> ships.size(), 0));
  buf -> Pack(label_build("Shield power", sol -> compute_shield_power(), 0));

  res -> Pack(frame("Military", buf));
  
  info_layout -> RemoveAll();
  info_layout -> Pack(frame("Solar " + sol -> id + " info", res), false, false);
}
 

