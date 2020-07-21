#include "solar_gui.hpp"

#include <memory>

#include "client_game.hpp"
#include "rsg/src/button.hpp"
#include "rsg/src/panel.hpp"
#include "rsg/src/utility.hpp"
#include "solar.hpp"
#include "style.hpp"
#include "types.hpp"
#include "utility.hpp"

using namespace std;
using namespace st3;
using namespace RSG;

typedef shared_ptr<list<string>> list_t;

PanelPtr column2() {
  return styled<Panel, list<ComponentPtr>, Panel::orientation>(
      {{"width", "50%"}},
      {},
      Panel::ORIENT_VERTICAL);
}

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
  PanelPtr p1 = column2();
  PanelPtr p2 = column2();
  list<ComponentPtr> children;

  for (auto v : keywords::development) {
    children.push_back(Button::create(
        v,
        [=](ButtonPtr b) {
          q->push_back(v);
          p2->clear_children();
          p2->add_child(building_queue(s, q));
        }));
  }

  p1->add_children(children);
  p2->add_child(building_queue(s, q));

  return styled<Panel, list<ComponentPtr>>({{"width", "50%"}}, {p1, p2});
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
  PanelPtr p1 = column2();
  PanelPtr p2 = column2();
  list<ComponentPtr> children;

  auto skey = utility::hm_keys(ship_stats::table());
  sort(skey.begin(), skey.end());
  for (auto v : skey) {
    if (r.can_build_ship(v, s)) {
      children.push_back(Button::create(
          v,
          [=](ButtonPtr b) {
            q->push_back(v);
            p2->clear_children();
            p2->add_child(ship_queue(q));
          }));
    }
  }

  p1->add_children(children);
  p2->add_child(ship_queue(q));

  return styled<Panel, list<ComponentPtr>>({{"width", "50%"}}, {p1, p2});
}

PanelPtr st3::solar_gui(solar_ptr s, research::data r, Voidfun on_cancel, function<void(list<string>, list<string>)> on_commit) {
  list_t bqueue(new list<string>(s->choice_data.building_queue));
  list_t squeue(new list<string>(s->choice_data.ship_queue));

  PanelPtr p = styled<Panel, list<ComponentPtr>, Panel::orientation>(
      main_panel_style,
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
