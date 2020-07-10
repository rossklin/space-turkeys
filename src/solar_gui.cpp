#include "solar_gui.h"

#include <memory>

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

typedef shared_ptr<list<string>> list_t;

PanelPtr building_queue(solar_ptr s, list_t q) {
  hm_t<string, int> available = s->development;
  list<ComponentPtr> children;
  PanelPtr p = Panel::create({}, Panel::ORIENT_VERTICAL);

  for (auto v : *q) {
    available[v]++;
    ButtonPtr b = Button::create(
        v + " level " + to_string(available[v]),
        [=](ButtonPtr self) {
          p->remove_child(self);
          auto i = find(q->begin(), q->end(), v);
          if (i != q->end()) q->erase(i);
        });

    children.push_back(b);
  }

  p->add_children(children);
  return p;
}

// Panel with buttons for queing development
PanelPtr make_left_panel(solar_ptr s, list_t q) {
  PanelPtr p1 = Panel::create({}, Panel::ORIENT_VERTICAL);
  PanelPtr p2 = Panel::create({}, Panel::ORIENT_VERTICAL);
  list<ComponentPtr> children;

  for (auto v : keywords::development) {
    children.push_back(Button::create(
        v,
        [=](ButtonPtr b) {
          // Run event handler through post process since we are adding/removing children
          q->push_back(v);
          p2->clear_children();
          p2->add_child(building_queue(s, q));
        }));
  }

  p1->add_children(children);
  p2->add_child(building_queue(s, q));

  return Panel::create({p1, p2});
}

PanelPtr ship_queue(list_t q) {
  list<ComponentPtr> children;
  PanelPtr p = Panel::create({}, Panel::ORIENT_VERTICAL);

  for (auto v : *q) {
    ButtonPtr b = Button::create(
        v,
        [=](ButtonPtr self) {
          p->remove_child(self);
          auto i = find(q->begin(), q->end(), v);
          if (i != q->end()) q->erase(i);
        });

    children.push_back(b);
  }

  p->add_children(children);
  return p;
}

// Panel with buttons for queing ship production
PanelPtr make_right_panel(solar_ptr s, research::data r, list_t q) {
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
            q->push_back(v);
            p2->clear_children();
            p2->add_child(ship_queue(q));
          }));
    }
  }

  p1->add_children(children);
  p2->add_child(ship_queue(q));

  return Panel::create({p1, p2});
}

PanelPtr solar_gui(solar_ptr s, research::data r, Voidfun on_cancel, function<void(list<string>, list<string>)> on_commit) {
  list_t bqueue(new list<string>(s->choice_data.building_queue));
  list_t squeue(new list<string>(s->choice_data.ship_queue));

  PanelPtr p = Panel::create(
      {
          make_label("Manage " + s->id),
          make_hbar(),
          Panel::create({
              make_left_panel(s, bqueue),
              make_right_panel(s, r, squeue),
          }),
          make_hbar(),
          Panel::create(
              {
                  Button::create("Cancel", [=](ButtonPtr s) { on_cancel(); }),
                  Button::create("Commit", [=](ButtonPtr s) { on_commit(*bqueue, *squeue); }),
              }),
      },
      Panel::ORIENT_VERTICAL);

  return p;
}
