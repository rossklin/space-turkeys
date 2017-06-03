
#include "development_tree.h"
#include "development_gui.h"
#include "research_gui.h"
#include "desktop.h"
#include "research.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace sfg;
using namespace interface;

const std::string research_gui::sfg_id = "research gui";

research_gui::Ptr research_gui::Create() {
  Ptr self(new research_gui());
  self -> Add(self -> layout);
  self -> SetAllocation(main_interface::qw_allocation);
  self -> SetId(sfg_id);
  return self;
}

// main window for research choice
research_gui::research_gui() : Window(Window::Style::BACKGROUND) {
  layout = Box::Create(Box::Orientation::VERTICAL);
  response = desktop -> response.research;

  // development gui
  development_gui::f_req_t f_req = [] (string v) -> list<string> {
    return desktop -> get_research().list_tech_requirements(v);
  };

  development_gui::f_select_t on_select = [this] (string result) {
    response = result;
  };

  hm_t<string, development::node> map;
  for (auto &f : research::data::table()) map[f.first] = f.second;
  layout -> Pack(development_gui::Create(map, response, f_req, on_select, false, main_interface::qw_allocation.width - 100));

  // accept/cancel
  Box::Ptr response_layout = Box::Create(Box::Orientation::HORIZONTAL);
  Button::Ptr b_ok = Button::Create("Accept");
  desktop -> bind_ppc(b_ok, [this] () {
      desktop -> response.research = response;
      desktop -> clear_qw();
    });
  response_layout -> Pack(b_ok);

  Button::Ptr b_cancel = Button::Create("Cancel");
  desktop -> bind_ppc(b_cancel, [this] () {
      desktop -> clear_qw();
    });
  response_layout -> Pack(b_cancel);

  layout -> Pack(response_layout);
}
