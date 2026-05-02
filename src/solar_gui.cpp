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

PanelPtr make_ship_buttons(solar_ptr s, shared_ptr<string> selected_ship, function<void()> on_change) {
  list<ComponentPtr> children;

  auto skey = utility::hm_keys(ship_stats::table());
  sort(skey.begin(), skey.end());
  for (auto v : skey) {
      string label = v;
      if (v == *selected_ship) label = "[ " + v + " ]";
      children.push_back(tag({"card"}, Button::create(label, [on_change, selected_ship, v]() {
        *selected_ship = v;
        on_change();
      })));
  }

  string label = "None";
  if ("" == *selected_ship) label = "[ None ]";
  children.push_back(tag({"card"}, Button::create(label, [on_change, selected_ship]() {
    *selected_ship = "";
    on_change();
  })));

  return Panel::create(children, Panel::ORIENT_VERTICAL);
}

PanelPtr st3::solar_gui(solar_ptr s, Voidfun on_cancel, function<void(string)> on_commit) {
  shared_ptr<string> selected_ship = make_shared<string>(s->choice_data.ship_to_build);

  PanelPtr p_ship_buttons = tag({"solar-component"}, Panel::create({}));

  function<void()> update_buttons;
  update_buttons = [=]() {
    p_ship_buttons->replace_children({make_ship_buttons(s, selected_ship, update_buttons)});
  };

  update_buttons();

  return Panel::create(
      {
          tag({"h1"}, Button::create("Manage " + s->id)),
          make_hbar(),
          tag(
              {"section", "solar-main-panel"},
              Panel::create({
                  tag({"solar-block"}, Panel::create({p_ship_buttons})),
              })),
          make_hbar(),
          tag(
              {"section"},
              Panel::create(
                  {
                      Button::create("Cancel", [=](ButtonPtr s) { on_cancel(); }),
                      Button::create("Commit", [=](ButtonPtr s) { on_commit(*selected_ship); }),
                  })),
      },
      Panel::ORIENT_VERTICAL);
}
