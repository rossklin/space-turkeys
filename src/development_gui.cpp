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

development_gui::Ptr development_gui::Create(hm_t<std::string, development::node> map, string sel, f_req_t f_req, f_select_t on_select, bool is_facility) {
  Ptr buf(new development_gui(map, f_req, on_select, is_facility));
  buf -> Add(buf -> frame);
  buf -> SetAllocation(main_interface::qw_allocation);
  buf -> SetId(sfg_id);
  return buf;
}

development_gui::development_gui(hm_t<std::string, development::node> _map, string _sel, f_req_t _f_req, f_select_t _on_select, bool is_facility) : Window(Window::Style::BACKGROUND) {
  map = _map;
  selected = _sel;
  f_req = _f_req;
  on_select = _on_select;

  for (auto &x : map) if (f_req(x.first).empty()) available.insert(x.first);

  for (auto &x : map) {
    if (!available.count(x.first)) {
      bool pass = true;
      if (is_facility) {
	for (auto f : x.second.depends_facilities) if (!available.count(f.first)) pass = false;
      } else {
	for (auto f : x.second.depends_techs) if (!available.count(f)) pass = false;
      }
      if (pass) dependent.insert(x.first);
    }
  }

  string title = is_facility ? "Select a facility to develop" : "Select a technology to develop";
  frame = Frame::Create(title);
  
  setup();
}

void development_gui::setup() {
  // main layout
  Box::Ptr layout = Box::Create(Box::Orientation::VERTICAL, 10);

  auto build_dependent = [this] (string v) -> Widget::Ptr {
    string text = v + ": " + boost::algorithm::join(f_req(v), ", ");
    return Label::Create(text);
  };

  auto build_available = [this] (string v) -> Widget::Ptr {
    development::node n = map[v];
    list<string> info;

    for (auto b : n.sector_boost) info.push_back(b.first + " + " + to_string((int)(100 * (b.second - 1))) + "%");
    for (auto su : n.ship_upgrades) info.push_back(su.first + " gains: " + boost::algorithm::join(su.second, ", "));

    auto b = Button::Create();
    b -> SetImage(Image(graphics::selector_card(v, v == selected, info)));
    desktop -> bind_ppc(b, [this, v] () {
	on_select(v);
	selected = v;
	setup();
      });
    
    return b;
  };

  // dependent techs go in a scrolled window
  ScrolledWindow::Ptr scrolledwindow = ScrolledWindow::Create();
  Box::Ptr window_box = Box::Create(Box::Orientation::VERTICAL, 5);

  scrolledwindow->SetScrollbarPolicy( ScrolledWindow::HORIZONTAL_NEVER | ScrolledWindow::VERTICAL_AUTOMATIC );
  scrolledwindow->AddWithViewport( window_box );

  for (auto v : dependent) window_box -> Pack(build_dependent(v));
  point req = window_box -> GetRequisition();
  scrolledwindow->SetRequisition(point(req.x, 200));
  layout -> Pack(scrolledwindow);

  Box::Ptr avl = Box::Create(Box::Orientation::HORIZONTAL);
  for (auto v : available) avl -> Pack(build_available(v));
  layout -> Pack(avl);

  frame -> RemoveAll();
  frame -> Add(layout);
}
