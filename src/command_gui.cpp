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

command_gui::Ptr command_gui::Create(client::command_selector::ptr c, client::game *g) {
  Ptr self(new command_gui(c, g));
  self -> Add(self -> layout);
  self -> SetAllocation(main_interface::qw_allocation);
  self -> SetId(sfg_id);
  return self;
}

command_gui::command_gui(client::command_selector::ptr c, client::game *g) : Window(Window::Style::BACKGROUND) {
  
  layout = Box::Create(Box::Orientation::VERTICAL, 10);
  // add header and policies
  string header = c -> source + ": " + c -> action + " " + c -> target;
  layout -> Pack(Label::Create(header));
  layout -> Pack(Separator::Create());

  hm_t<int, RadioButton::Ptr> policy;
  policy[fleet::policy_reasonable] = RadioButton::Create("Reasonable");
  policy[fleet::policy_aggressive] = RadioButton::Create("Aggressive", policy[fleet::policy_reasonable] -> GetGroup());
  policy[fleet::policy_evasive] = RadioButton::Create("Evasive", policy[fleet::policy_reasonable] -> GetGroup());
  policy[fleet::policy_maintain_course] = RadioButton::Create("Maintain Course", policy[fleet::policy_reasonable] -> GetGroup());

  selected_policy = c -> policy;
  policy[selected_policy] -> SetActive(true);

  for (auto p : policy) {
    int fp = p.first;
    p.second -> GetSignal(ToggleButton::OnToggle).Connect([this, fp] () {
	selected_policy = fp;
    });
    layout -> Pack(p.second);
  }

  layout -> Pack(Separator::Create());

  for (auto sid : g -> get_ready_ships(c -> source)) {
    ship::ptr s = g -> get_specific<ship>(sid);
    data[s -> ship_class].available++;
  }

  for (auto sid : c -> ships) {
    ship::ptr s = g -> get_specific<ship>(sid);
    data[s -> ship_class].allocated++;
  }

  point label_dims(80, 40);

  auto build_ship_label = [this, label_dims] (string ship_class) -> sf::Image {
    string label = to_string(data[ship_class].allocated) + "/" + to_string(data[ship_class].allocated + data[ship_class].available);
    return graphics::ship_image_label(label, ship_class, label_dims.x, label_dims.y);
  };

  // build ship allocation tables for each available ships class
  for (string ship_class : ship::all_classes()) {
    int total = data[ship_class].available + data[ship_class].allocated;
    if (total == 0) continue;

    Box::Ptr box = Box::Create(Box::Orientation::HORIZONTAL, 10);
    Scale::Ptr scale = Scale::Create( sfg::Scale::Orientation::HORIZONTAL);
    data[ship_class].image = Image::Create(build_ship_label(ship_class));
    data[ship_class].adjust = scale -> GetAdjustment();
    data[ship_class].adjust -> SetLower(0);
    data[ship_class].adjust -> SetUpper(total);
    data[ship_class].adjust -> SetValue(data[ship_class].allocated);
    data[ship_class].adjust -> GetSignal(Adjustment::OnChange).Connect([this, build_ship_label, ship_class, total] () {
	data[ship_class].allocated = data[ship_class].adjust -> GetValue();
	data[ship_class].available = total - data[ship_class].allocated;
	data[ship_class].image -> SetImage(build_ship_label(ship_class));
      });

    box -> Pack(data[ship_class].image);
    box -> Pack(scale);
    layout -> Pack(box);
  }

  // add ok and cancel
  Button::Ptr ok = Button::Create("OK");
  auto on_ok = [this, c, g] () {
    c -> policy = selected_policy;
      
    // assign correct number of ships per class to c
    c -> ships.clear();
    set<combid> ready_ships = g -> get_ready_ships(c -> source);

    // first list ships by class
    hm_t<string, set<combid> > by_class;
    for (auto sid : ready_ships) {
      ship::ptr s = g -> get_specific<ship>(sid);
      by_class[s -> ship_class].insert(s -> id);
    }

    // then assign ships
    for (auto x : data) {
      string ship_class = x.first;
      int count = x.second.allocated;
      for (int i = 0; i < count; i++) {
	auto iter = by_class[ship_class].begin();
	combid sid = *iter;
	by_class[ship_class].erase(iter);
	c -> ships.insert(sid);
      }
    }
      
    desktop -> clear_qw();
  };
  desktop -> bind_ppc(ok, on_ok);

  Button::Ptr cancel = Button::Create("Cancel");
  desktop -> bind_ppc(cancel, [] () {desktop -> clear_qw();});

  Button::Ptr b_delete = Button::Create("Delete");
  desktop -> bind_ppc(b_delete, [g, c] () {
      desktop -> clear_qw();
      g -> remove_command(c -> id);
    });

  Box::Ptr l_bottom = Box::Create(Box::Orientation::HORIZONTAL, 10);
  l_bottom -> Pack(ok);
  l_bottom -> Pack(cancel);
  l_bottom -> Pack(b_delete);
  layout -> Pack(l_bottom);
}
