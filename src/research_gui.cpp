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


// main window for research choice
Widget::Ptr research_gui() {
  string response = desktop -> response.research;

  // development gui
  choice_gui::f_info_t f_info = [] (string v) -> choice_info {
    list<string> res = desktop -> get_research().list_tech_requirements(v);
    if (desktop -> get_research().researched().count(v)) res.push_back("Already researched");
    return res;
  };

  choice_gui::f_result_t callback = [] (string result, bool accepted) {
    response = result;
  };

  hm_t<string, development::node> map;
  for (auto &f : research::data::table()) map[f.first] = f.second;
  for (auto &t : desktop -> get_research().tech_map) map[t.first] = t.second;
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
