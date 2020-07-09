#include "solar_gui.h"

#include "client_game.h"
#include "rsg/src/button.hpp"
#include "rsg/src/panel.hpp"
#include "rsg/src/utility.hpp"
#include "solar.h"
#include "types.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace interface;
using namespace RSG;

list<string> bqueue;
list<string> squeue;

PanelPtr building_queue(solar_ptr s) {
  hm_t<string, int> available = s->development;
  list<ComponentPtr> children;
  PanelPtr p = Panel::create({}, Panel::ORIENT_VERTICAL);

  for (auto v : bqueue) {
    available[v]++;
    ButtonPtr b = Button::create(
        v + " level " + to_string(available[v]),
        [=](ButtonPtr self) {
          p->remove_child(self);
          auto i = find(bqueue.begin(), bqueue.end(), v);
          if (i != bqueue.end()) bqueue.erase(i);
        });

    children.push_back(b);
  }

  p->add_children(children);
  return p;
}

// Panel with buttons for queing development
PanelPtr make_left_panel(solar_ptr s) {
  PanelPtr p1 = Panel::create({}, Panel::ORIENT_VERTICAL);
  PanelPtr p2 = Panel::create({}, Panel::ORIENT_VERTICAL);
  list<ComponentPtr> children;

  for (auto v : keywords::development) {
    children.push_back(Button::create(
        v,
        [=](ButtonPtr b) {
          // Run event handler through post process since we are adding/removing children
          bqueue.push_back(v);
          p2->clear_children();
          p2->add_child(building_queue(s));
        }));
  }

  p1->add_children(children);
  p2->add_child(building_queue(s));

  return Panel::create({p1, p2});
}

PanelPtr ship_queue() {
  list<ComponentPtr> children;
  PanelPtr p = Panel::create({}, Panel::ORIENT_VERTICAL);

  for (auto v : squeue) {
    ButtonPtr b = Button::create(
        v,
        [=](ButtonPtr self) {
          p->remove_child(self);
          auto i = find(bqueue.begin(), bqueue.end(), v);
          if (i != bqueue.end()) bqueue.erase(i);
        });

    children.push_back(b);
  }

  p->add_children(children);
  return p;
}

// Panel with buttons for queing ship production
PanelPtr make_right_panel(solar_ptr s, research::data r) {
  PanelPtr p1 = Panel::create({}, Panel::ORIENT_VERTICAL);
  PanelPtr p2 = Panel::create({}, Panel::ORIENT_VERTICAL);
  list<ComponentPtr> children;

  auto skey = utility::hm_keys(ship_stats::table());
  sort(skey.begin(), skey.end());
  for (auto v : skey) {
    if (r.can_build_ship(v, s)) {
      children.push_back(Button::create(
          v,
          [=](ButtonPtr b) {
            // Run event handler through post process since we are adding/removing children
            bqueue.push_back(v);
            p2->clear_children();
            p2->add_child(building_queue(s));
          }));
    }
  }

  p1->add_children(children);
  p2->add_child(ship_queue());

  return Panel::create({p1, p2});
}

PanelPtr solar_gui(solar_ptr s, research::data r, Voidfun on_cancel, function<void(list<string>, list<string>)> on_commit) {
  function<ButtonPtr(string)> make_label = styled_generator<Button, string>({
      {"background-color", "00000000"},
      {"border-thickness", "0"},
  });

  auto make_hbar = styled_generator<Button>({
      {"width", "100%"},
      {"height", "1px"},
      {"border-thickness", "0"},
      {"background-color", "112233ff"},
      {"margin-top", "20"},
      {"margin-bottom", "20"},
      {"margin-left", "0"},
      {"margin-right", "0"},
  });

  PanelPtr p = Panel::create(
      {
          make_label("Manage " + s->id),
          make_hbar(),
          Panel::create({
              make_left_panel(s),
              make_right_panel(s, r),
          }),
          make_hbar(),
          Panel::create(
              {
                  Button::create("Cancel", [=](ButtonPtr s) { on_cancel(); }),
                  Button::create("Commit", [=](ButtonPtr s) { on_commit(bqueue, squeue); }),
              }),
      },
      Panel::ORIENT_VERTICAL);

  return p;
}
