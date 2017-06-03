#include <set>
#include <boost/algorithm/string/join.hpp>

#include "desktop.h"
#include "development_tree.h"
#include "development_gui.h"
#include "graphics.h"

using namespace std;
using namespace st3;
using namespace sfg;
using namespace interface;

const string development_gui::sfg_id = "development_gui";

development_gui::Ptr development_gui::Create(hm_t<std::string, development::node> map, string sel, f_req_t f_req, f_select_t on_select, bool is_facility, int w) {
  Ptr buf(new development_gui(map, sel, f_req, on_select, is_facility, w));
  buf -> Add(buf -> layout);
  buf -> SetId(sfg_id);
  return buf;
}

development_gui::development_gui(hm_t<std::string, development::node> _map, string _sel, f_req_t _f_req, f_select_t _on_select, bool is_facility, int w) : Bin() {
  map = _map;
  selected = _sel;
  f_req = _f_req;
  on_select = _on_select;
  width = w;

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

  layout = Box::Create(Box::Orientation::VERTICAL, 5);
    
  setup();
}

const string& development_gui::GetName() const {
  return sfg_id;
}

sf::Vector2f development_gui::CalculateRequisition() {
  return layout -> GetRequisition();
}

void development_gui::setup() {
  layout -> RemoveAll();

  // show progress
  Frame::Ptr pf = Frame::Create("Progress");
  int percent = 0;

  // todo: fun set_progress
  layout -> Pack(pf);

  string title = is_facility ? "Select a facility to build" : "Select a technology to develop";
  frame = Frame::Create(title);

  // inner layout
  Box::Ptr buf = Box::Create(Box::Orientation::VERTICAL, 10);

  auto build = [this] (string v, bool available) -> Widget::Ptr {
    development::node n = map[v];
    list<string> info;

    for (auto b : n.sector_boost) info.push_back(b.first + " + " + to_string((int)(100 * (b.second - 1))) + "%");
    for (auto su : n.ship_upgrades) info.push_back(su.first + " gains: " + boost::algorithm::join(su.second, ", "));

    auto b = Button::Create();
    b -> SetImage(Image::Create(graphics::selector_card(v, v == selected, info, f_req(v))));

    if (available) {
      desktop -> bind_ppc(b, [this, v] () {
	  on_select(v);
	  selected = v;
	  setup();
	});
    }
    
    return b;
  };

  // devs go in a scrolled window
  Box::Ptr window_box = Box::Create(Box::Orientation::HORIZONTAL, 5);
  for (auto v : available) window_box -> Pack(build(v, true));
  for (auto v : dependent) window_box -> Pack(build(v, false));
  buf -> Pack(graphics::wrap_in_scroll(window_box, true, width));
  frame -> Add(buf);

  layout -> Pack(frame);
}
