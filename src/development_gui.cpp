#include <set>
#include <boost/algorithm/string/join.hpp>

#include "desktop.h"
#include "development_tree.h"
#include "development_gui.h"

using namespace std;
using namespace st3;
using namespace sfg;
using namespace interface;

const string development_gui::sfg_id = "development_gui";

development_gui::Ptr development_gui::Create(hm_t<std::string, development::node> map, f_req_t f_req, f_complete_t on_complete, bool is_facility) {
  Ptr buf(new development_gui(map, f_req, on_complete, is_facility));
  buf -> Add(buf -> frame);
  buf -> SetAllocation(main_interface::qw_allocation);
  buf -> SetId(sfg_id);
  return buf;
}

development_gui::development_gui(hm_t<std::string, development::node> map, f_req_t f_req, f_complete_t on_complete, bool is_facility) : Window(Window::Style::BACKGROUND) {

  set<string> available;
  for (auto &x : map) if (f_req(x.first).empty()) available.insert(x.first);

  set<string> dependent;
  for (auto &x : map) {
    if (!available.count(x.first)) {
      if (is_facility) {
	for (auto f : x.second.depends_facilities) if (available.count(f.first)) dependent.insert(f.first);
      } else {
	for (auto f : x.second.depends_techs) if (available.count(f)) dependent.insert(f);	
      }
    }
  }

  // main layout
  string title = is_facility ? "Select a facility to develop" : "Select a technology to develop";
  frame = Frame::Create(title);
  Box::Ptr layout = Box::Create(Box::Orientation::VERTICAL, 10);

  auto build_dependent = [f_req] (string v) -> Widget::Ptr {
    string text = v + ": " + boost::algorithm::join(f_req(v), ", ");
    return Label::Create(text);
  };

  auto build_available = [on_complete,&map] (string v) -> Widget::Ptr {
    development::node n = map[v];
    Box::Ptr l = Box::Create(Box::Orientation::VERTICAL);
    Frame::Ptr f = Frame::Create(v);

    for (auto b : n.sector_boost) l -> Pack(Label::Create(b.first + " + " + to_string(100 * (b.second - 1)) + "%"));
    for (auto su : n.ship_upgrades) l -> Pack(Label::Create(su.first + " gains: " + boost::algorithm::join(su.second, ", ")));
    
    f -> Add(l);
    f -> GetSignal(Widget::OnLeftClick).Connect([v, on_complete] () {on_complete(true, v);});
    return f;
  };

  for (auto v : dependent) layout -> Pack(build_dependent(v));
  Box::Ptr avl = Box::Create(Box::Orientation::HORIZONTAL);
  for (auto v : available) avl -> Pack(build_available(v));
  layout -> Pack(avl);

  Button::Ptr b_cancel = Button::Create("Cancel");
  b_cancel -> GetSignal(Widget::OnLeftClick).Connect([on_complete] () {on_complete(false, "");});
  layout -> Pack(b_cancel);

  frame -> Add(layout);
}
