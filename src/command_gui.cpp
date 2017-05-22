#include <cstdlib>
#include <string>
#include <iostream>
#include <unordered_map>
#include <set>
#include <utility>

#include "selector.h"
#include "command_gui.h"
#include "utility.h"
#include "graphics.h"
#include "solar.h"
#include "client_game.h"
#include "desktop.h"
#include "fleet.h"

using namespace std;
using namespace st3;
using namespace sfg;
using namespace interface;

const std::string command_gui::sfg_id = "command gui";

void command_gui::table_t::setup(int ncols, float pad) {
  layout = Box::Create(Box::Orientation::VERTICAL, padding);
  cols = ncols;
  padding = pad;
}

void command_gui::table_t::update_rows() {
  int n = buttons.size();
  layout -> RemoveAll();
  rows.clear();

  if (n == 0) return;
  
  rows.resize(ceil(n / cols));
  
  int i = 0;
  for (i = 0; i < rows.size(); i++) {
    rows[i] = Box::Create(Box::Orientation::HORIZONTAL, padding);
    layout -> Pack(rows[i]);
  }

  int count = 0;
  for (auto b : buttons) {
    rows[count / cols] -> Pack(b);
    count++;
  }
}

void command_gui::table_t::add_button(sfg::Button::Ptr b) {
  buttons.insert(b);
  update_rows();
}

void command_gui::table_t::remove_button(sfg::Button::Ptr b) {
  buttons.erase(b);
  update_rows();
}

command_gui::Ptr command_gui::Create(client::command_selector::ptr c, client::game *g) {
  Ptr self(new command_gui(c, g));
  self -> Add(self -> layout);
  self -> SetAllocation(main_interface::qw_allocation);
  self -> SetId(sfg_id);
  return self;
}

command_gui::command_gui(client::command_selector::ptr c, client::game *g) : Window(Window::Style::BACKGROUND) {
  // dimensions
  sf::FloatRect bounds = main_interface::qw_allocation;
  
  layout = Box::Create(Box::Orientation::VERTICAL, 10);
  // add header and policies
  string header = c -> action + ": from " + c -> source + " to " + c -> target;
  layout -> Pack(Label::Create(header));
  layout -> Pack(Separator::Create());

  hm_t<int, RadioButton::Ptr> policy;
  policy[fleet::policy_reasonable] = RadioButton::Create("Reasonable");
  policy[fleet::policy_aggressive] = RadioButton::Create("Aggressive", policy[fleet::policy_reasonable] -> GetGroup());
  policy[fleet::policy_evasive] = RadioButton::Create("Evasive", policy[fleet::policy_reasonable] -> GetGroup());
  policy[fleet::policy_maintain_course] = RadioButton::Create("Maintain Course", policy[fleet::policy_reasonable] -> GetGroup());

  policy[c -> policy] -> SetActive(true);

  for (auto p : policy) {
    int fp = p.first;
    p.second -> GetSignal(ToggleButton::OnToggle).Connect([c, fp] () {
	c -> policy = fp;
    });
    layout -> Pack(p.second);
  }

  layout -> Pack(Separator::Create());

  // Add ship buttons
  float h_header = layout -> GetRequisition().y + 40;
  float padding = 5;
  sf::FloatRect table_bounds(bounds.left + padding, bounds.top + 2 * padding + h_header, bounds.width - 2 * padding, bounds.height - 3 * padding - h_header);

  
  // setup command gui using ready ships from source plus ships
  // already selected for this command (they will not be listed as
  // ready)
  set<combid> ready_ships = g -> get_ready_ships(c -> source);
  ready_ships += c -> ships;

  hm_t<string, set<combid> > by_class;

  for (auto sid : ready_ships) {
    ship::ptr s = g -> get_specific<ship>(sid);
    by_class[s -> ship_class].insert(s -> id);
  }

  sf::FloatRect t_rect = table_bounds;
  t_rect.height /= by_class.size();

  // build ship allocation tables for each available ships class
  for (auto data : by_class) {
    string ship_class = data.first;
    set<combid> ships = data.second;
    Box::Ptr box = Box::Create(Box::Orientation::HORIZONTAL, 10);

    float inner_width = (t_rect.width - 3 * padding) / 2;
    float inner_height = t_rect.height - 2 * padding;
    int n_total = ships.size();
    int cols = ceil(sqrt(n_total));
    int rows = ceil(n_total / cols);
    point b_dims(inner_width / cols - 2 * padding, inner_height / rows - 2 * padding);

    tab_available[ship_class].setup(cols, 2);
    tab_allocated[ship_class].setup(cols, 2);

    // insert ship buttons
    for (auto sid : ships) {
      Button::Ptr b = graphics::ship_button(ship_class, b_dims.x, b_dims.y);

      b -> GetSignal(Widget::OnLeftClick).Connect([this, ship_class, sid, c] () {
	  Button::Ptr p = ship_buttons[sid];
	  if (c -> ships.count(sid)) {
	    c -> ships.erase(sid);
	    tab_allocated[ship_class].remove_button(p);
	    tab_available[ship_class].add_button(p);
	  }else{
	    c -> ships.insert(sid);
	    tab_available[ship_class].remove_button(p);
	    tab_allocated[ship_class].add_button(p);
	  }
	});

      ship_buttons[sid] = b;
      if (c -> ships.count(sid)) {
	tab_allocated[ship_class].add_button(b);
      } else {
	tab_available[ship_class].add_button(b);
      }
    }

    // set allocations
    sf::FloatRect inner_bounds(t_rect.left + padding, t_rect.top + padding, inner_width, inner_height);
    tab_available[ship_class].layout -> SetAllocation(inner_bounds);
    inner_bounds.left = t_rect.left + inner_width + 2 * padding;
    tab_allocated[ship_class].layout -> SetAllocation(inner_bounds);

    box -> Pack(tab_available[ship_class].layout);
    box -> Pack(tab_allocated[ship_class].layout);
    box -> SetAllocation(t_rect);

    layout -> Pack(box);
    t_rect.top += t_rect.height;
  }

  // add ok
  Button::Ptr ok = Button::Create("OK");
  ok -> GetSignal(Widget::OnLeftClick).Connect([] () {
      desktop -> clear_qw();
    });
  layout -> Pack(ok);
}
