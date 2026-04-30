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

PanelPtr make_building_queue(solar_ptr s, list_t q) {
  hm_t<string, int> available = s->development;
  list<ComponentPtr> children = {make_label("Queue"), make_hbar()};
  PanelPtr p = tag({"solar-queue"}, Panel::create({}, Panel::ORIENT_VERTICAL));

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

PanelPtr make_building_buttons(function<void(string)> callback) {
  list<ComponentPtr> children;
  for (auto v : keywords::development) {
    children.push_back(tag({"card"}, Button::create(v, [callback, v]() { callback(v); })));
  }
  return Panel::create(children, Panel::ORIENT_VERTICAL);
}

PanelPtr make_ship_queue(list_t q) {
  list<ComponentPtr> children = {make_label("Queue"), make_hbar()};
  PanelPtr p = tag({"solar-queue"}, Panel::create({}, Panel::ORIENT_VERTICAL));

  for (auto v : *q) {
    ButtonPtr b = tag(
        {"label"},
        Button::create(
            v,
            [=](ButtonPtr self) {
              p->remove_child(self);
              auto i = find(q->begin(), q->end(), v);
              if (i != q->end()) q->erase(i);
            }));

    children.push_back(b);
  }

  p->add_children(children);
  return p;
}

PanelPtr make_ship_buttons(solar_ptr s, research::data r, function<void(string)> callback) {
  list<ComponentPtr> children;

  auto skey = utility::hm_keys(ship_stats::table());
  sort(skey.begin(), skey.end());
  for (auto v : skey) {
    if (r.can_build_ship(v, s)) {
      children.push_back(tag({"card"}, Button::create(v, [callback, v]() { callback(v); })));
    }
  }

  return Panel::create(children, Panel::ORIENT_VERTICAL);
}

PanelPtr st3::solar_gui(solar_ptr s, research::data r, Voidfun on_cancel, function<void(list<string>, list<string>)> on_commit) {
  list_t bqueue(new list<string>(s->choice_data.building_queue));
  list_t squeue(new list<string>(s->choice_data.ship_queue));

  PanelPtr p_building_buttons, p_building_queue, p_ship_buttons, p_ship_queue;

  p_building_queue = tag({"solar-component"}, Panel::create({make_building_queue(s, bqueue)}));
  p_ship_queue = tag({"solar-component"}, Panel::create({make_ship_queue(squeue)}));

  auto ship_button_callback = [=](string v) {
    squeue->push_back(v);
    p_ship_queue->replace_children({make_ship_queue(squeue)});
  };

  p_ship_buttons = tag({"solar-component"}, make_ship_buttons(s, r, ship_button_callback));

  auto building_button_callback = [=](string v) {
    bqueue->push_back(v);
    p_building_queue->replace_children({make_building_queue(s, bqueue)});
  };

  p_building_buttons = tag({"solar-component"}, make_building_buttons(building_button_callback));

  return Panel::create(
      {
          tag({"h1"}, Button::create("Manage " + s->id)),
          make_hbar(),
          tag(
              {"section", "solar-main-panel"},
              Panel::create({
                  tag({"solar-block"}, Panel::create({p_building_buttons, p_building_queue})),
                  tag({"solar-block"}, Panel::create({p_ship_buttons, p_ship_queue})),
              })),
          make_hbar(),
          tag(
              {"section"},
              Panel::create(
                  {
                      Button::create("Cancel", [=](ButtonPtr s) { on_cancel(); }),
                      Button::create("Commit", [=](ButtonPtr s) { on_commit(*bqueue, *squeue); }),
                  })),
      },
      Panel::ORIENT_VERTICAL);
}
