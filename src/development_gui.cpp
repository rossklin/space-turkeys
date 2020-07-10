#include "development_gui.hpp"

#include <boost/algorithm/string/join.hpp>
#include <set>

#include "desktop.hpp"
#include "development_tree.hpp"
#include "graphics.hpp"

using namespace std;
using namespace st3;
using namespace sfg;
using namespace interface;

const string development_gui::sfg_id = "development_gui";

development_gui::Ptr development_gui::Create(hm_t<std::string, development::node> map, string sel, f_req_t f_req, f_select_t on_select, bool is_facility, int w) {
  Ptr buf(new development_gui(map, sel, f_req, on_select, is_facility, w));
  buf->Add(buf->frame);
  buf->SetId(sfg_id);
  return buf;
}

development_gui::development_gui(hm_t<std::string, development::node> _map, string _sel, f_req_t _f_req, f_select_t _on_select, bool is_facility, int w) : Bin() {
  map = _map;
  selected = _sel;
  f_req = _f_req;
  on_select = _on_select;
  width = w;

  for (auto &x : map)
    if (f_req(x.first).empty()) available.insert(x.first);

  for (auto &x : map) {
    if (!available.count(x.first)) dependent.insert(x.first);
  }

  string title = is_facility ? "Select a facility to build" : "Select a technology to develop";
  frame = Frame::Create(title);

  layout = Box::Create(Box::Orientation::VERTICAL, 5);
  frame->Add(layout);

  setup();
}

const string &development_gui::GetName() const {
  return sfg_id;
}

sf::Vector2f development_gui::CalculateRequisition() {
  return frame->GetRequisition();
}

void development_gui::reset_image(string v) {
  if (!button_map.count(v)) return;

  list<string> info;
  development::node n = map[v];

  for (auto b : n.sector_boost) info.push_back(b.first + " + " + to_string((int)(100 * (b.second - 1))) + "%");
  for (auto su : n.ship_upgrades) info.push_back(su.first + " gains: " + boost::algorithm::join(su.second, ", "));

  button_map[v]->SetImage(Image::Create(graphics::selector_card(v, v == selected, n.progress / n.cost_time, info, f_req(v))));
}

void development_gui::setup() {
  layout->RemoveAll();

  auto build = [this](string v, bool available) -> Widget::Ptr {
    auto b = Button::Create();
    button_map[v] = b;
    reset_image(v);

    if (available) {
      desktop->bind_ppc(b, [this, v]() {
        // callback
        on_select(v);

        // update button images
        string previous = selected;
        selected = v;
        reset_image(v);
        reset_image(previous);
      });
    }

    return b;
  };

  // devs go in a scrolled window
  Box::Ptr window_box = Box::Create(Box::Orientation::HORIZONTAL, 5);
  for (auto v : available) window_box->Pack(build(v, true));
  for (auto v : dependent) window_box->Pack(build(v, false));
  layout->Pack(graphics::wrap_in_scroll(window_box, true, width));
}
