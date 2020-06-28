#include "command_gui.h"

#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

#include "client_game.h"
#include "desktop.h"
#include "fleet.h"
#include "graphics.h"
#include "selector.h"
#include "solar.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace sfg;
using namespace interface;

const std::string command_gui::sfg_id = "command gui";

command_gui::Ptr command_gui::Create(client::command_selector::ptr c, client::game *g) {
  Ptr self(new command_gui(c, g));
  self->Add(self->layout);
  self->SetAllocation(main_interface::qw_allocation);
  self->SetId(sfg_id);
  return self;
}

command_gui::command_gui(client::command_selector::ptr c, client::game *g) : Window(Window::Style::BACKGROUND) {
  layout = Box::Create(Box::Orientation::VERTICAL, 10);
  // add header and policies
  string header = c->source + ": " + c->action + " " + c->target;
  layout->Pack(Label::Create(header));
  layout->Pack(Separator::Create());

  hm_t<int, RadioButton::Ptr> policy;
  policy[fleet::policy_aggressive] = RadioButton::Create("Aggressive");
  policy[fleet::policy_evasive] = RadioButton::Create("Evasive", policy[fleet::policy_aggressive]->GetGroup());
  policy[fleet::policy_maintain_course] = RadioButton::Create("Maintain Course", policy[fleet::policy_aggressive]->GetGroup());

  selected_policy = c->policy;
  policy[selected_policy]->SetActive(true);

  for (auto p : policy) {
    int fp = p.first;
    p.second->GetSignal(ToggleButton::OnToggle).Connect([this, fp]() {
      selected_policy = fp;
    });
    layout->Pack(p.second);
  }

  layout->Pack(Separator::Create());

  for (auto sid : g->get_ready_ships(c->source)) {
    ship::ptr s = g->get_specific<ship>(sid);
    data[s->ship_class].available++;
  }

  for (auto sid : c->ships) {
    ship::ptr s = g->get_specific<ship>(sid);
    data[s->ship_class].allocated++;
  }

  point label_dims(80, 40);

  auto build_ship_label = [this](string ship_class) -> string {
    return to_string(data[ship_class].allocated) + "/" + to_string(data[ship_class].allocated + data[ship_class].available);
  };

  // build ship allocation tables for each available ships class
  Box::Ptr ships_layout = Box::Create(Box::Orientation::VERTICAL, 5);
  for (string ship_class : ship::all_classes()) {
    int total = data[ship_class].available + data[ship_class].allocated;
    if (total == 0) continue;

    Box::Ptr box = Box::Create(Box::Orientation::HORIZONTAL, 10);
    Scale::Ptr scale = Scale::Create(sfg::Scale::Orientation::HORIZONTAL);
    scale->SetRequisition(sf::Vector2f(400, 10));
    data[ship_class].alloc_label = Label::Create(build_ship_label(ship_class));
    data[ship_class].adjust = scale->GetAdjustment();
    data[ship_class].adjust->SetLower(0);
    data[ship_class].adjust->SetUpper(total);
    data[ship_class].adjust->SetValue(data[ship_class].allocated);
    data[ship_class].adjust->GetSignal(Adjustment::OnChange).Connect([this, g, build_ship_label, ship_class, total]() {
      int value = min((int)data[ship_class].adjust->GetValue(), g->get_max_ships_per_fleet(g->self_id));
      data[ship_class].allocated = value;
      data[ship_class].available = total - data[ship_class].allocated;
      data[ship_class].alloc_label->SetText(build_ship_label(ship_class));

      // set all other ship classes to 0
      for (auto sc2 : ship::all_classes()) {
        if (sc2 == ship_class) continue;
        // data[sc2].available += data[sc2].allocated;
        // data[sc2].allocated = 0;
        data[sc2].adjust->SetValue(0);
      }
    });

    box->Pack(Label::Create(ship_class));
    box->Pack(scale);
    box->Pack(data[ship_class].alloc_label);
    ships_layout->Pack(box);
  }

  layout->Pack(graphics::wrap_in_scroll(ships_layout, false, 300));

  // add ok and cancel
  Button::Ptr ok = Button::Create("OK");
  auto on_ok = [this, c, g]() {
    c->policy = selected_policy;

    // assign correct number of ships per class to c
    c->ships.clear();
    set<combid> ready_ships = g->get_ready_ships(c->source);

    // first list ships by class
    hm_t<string, set<combid> > by_class;
    for (auto sid : ready_ships) {
      ship::ptr s = g->get_specific<ship>(sid);
      by_class[s->ship_class].insert(s->id);
    }

    // then assign ships
    for (auto x : data) {
      string ship_class = x.first;
      int count = x.second.allocated;
      for (int i = 0; i < count; i++) {
        auto iter = by_class[ship_class].begin();
        combid sid = *iter;
        by_class[ship_class].erase(iter);
        c->ships.insert(sid);
      }
    }

    desktop->clear_qw();
  };
  desktop->bind_ppc(ok, on_ok);

  Button::Ptr cancel = Button::Create("Cancel");
  desktop->bind_ppc(cancel, []() { desktop->clear_qw(); });

  Button::Ptr b_delete = Button::Create("Delete");
  desktop->bind_ppc(b_delete, [g, c]() {
    desktop->clear_qw();
    g->remove_command(c->id);
  });

  Box::Ptr l_bottom = Box::Create(Box::Orientation::HORIZONTAL, 10);
  l_bottom->Pack(ok);
  l_bottom->Pack(cancel);
  l_bottom->Pack(b_delete);
  layout->Pack(l_bottom);
}
