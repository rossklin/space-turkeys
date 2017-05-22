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

void insert_into_table(Widget::Ptr b, Table::Ptr t, int cols) {
  int i = t -> GetChildren().size();
  int r = i / cols;
  int c = i % cols;
  int opt_x = Table::FILL | Table::EXPAND;
  int opt_y = Table::FILL | Table::EXPAND;
  point padding(5,5);
  t -> Attach(b, sf::Rect<sf::Uint32>(c, r, 1, 1), opt_x, opt_y, padding);
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

  for (auto p : policy) {
    p.second -> GetSignal(ToggleButton::OnToggle).Connect([c, p] () {
	c -> policy = p.first;
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

  for (auto data : by_class) {
    string ship_class = data.first;
    set<combid> ships = data.second;
    Box::Ptr box = Box::Create(Box::Orientation::HORIZONTAL, 10);

    tab_available[ship_class] = Table::Create();
    tab_allocated[ship_class] = Table::Create();
    
    int n_total = ships.size();
    int cols = ceil(sqrt(n_total));
    int rows = ceil(n_total / cols);
    point b_dims(0.5 * t_rect.width / cols, t_rect.height / cols);

    // insert ship buttons
    for (auto sid : ships) {
      Button::Ptr b = graphics::ship_button(ship_class, b_dims.x, b_dims.y);

      b -> GetSignal(Widget::OnLeftClick).Connect([this, ship_class, sid, c, cols] () {
	  Button::Ptr p = ship_buttons[sid];
	  if (c -> ships.count(sid)) {
	    c -> ships.erase(sid);
	    tab_allocated[ship_class] -> Remove(p);
	    insert_into_table(p, tab_available[ship_class], cols);
	  }else{
	    c -> ships.insert(sid);
	    tab_available[ship_class] -> Remove(p);
	    insert_into_table(p, tab_allocated[ship_class], cols);
	  }
	});

      ship_buttons[sid] = b;
      if (c -> ships.count(sid)) {
	insert_into_table(b, tab_allocated[ship_class], cols);
      } else {
	insert_into_table(b, tab_available[ship_class], cols);
      }
    }

    box -> Pack(tab_available[ship_class]);
    box -> Pack(tab_allocated[ship_class]);
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
