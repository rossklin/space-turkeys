#include "target_gui.hpp"

#include <functional>
#include <list>
#include <map>
#include <string>

#include "rsg/src/button.hpp"
#include "rsg/src/button_options.hpp"
#include "rsg/src/component.hpp"
#include "rsg/src/panel.hpp"
#include "rsg/src/utility.hpp"
#include "style.hpp"
#include "utility.hpp"

using namespace std;
using namespace RSG;
using namespace st3;
using namespace st3::utility;

PanelPtr st3::target_gui(
    sf::Vector2f position,
    hm_t<std::string, std::set<idtype>> action_targets,
    std::function<void(std::string action, idtype target)> callback,
    RSG::Voidfun on_cancel) {
  shared_ptr<string> sel_action = make_shared<string>("");
  shared_ptr<idtype> sel_target = make_shared<idtype>(identifier::no_entity);

  ButtonPtr b_cancel = Button::create("Cancel", on_cancel);
  ButtonPtr b_commit = Button::create("Commit", [sel_action, sel_target, callback]() { callback(*sel_action, *sel_target); });
  b_commit->add_state("disabled");

  auto t_opts = ButtonOptions::create(
      list<string>{},
      [sel_target, b_commit](string k) {
        *sel_target = stoi(k);
        b_commit->remove_state("disabled");
      },
      ORIENT_VERTICAL);
  t_opts->add_state("invisible");

  auto a_opts = ButtonOptions::create(
      range_init<list<string>>(hm_keys(action_targets)),
      [=](string k) {
        if (*sel_action == k) return;

        *sel_action = k;
        *sel_target = identifier::no_entity;
        b_commit->add_state("disabled");

        map<string, ButtonPtr> buf;
        for (auto v : action_targets.at(k)) {
          string label = v == identifier::no_entity ? "Add waypoint" : to_string(v);
          buf[to_string(v)] = Button::create(label);
        }
        t_opts->set_children(buf);
        t_opts->remove_state("invisible");
      },
      ORIENT_VERTICAL);

  return tag(
      {"target-gui"},
      Panel::create(
          {
              tag(
                  {"section", "target-wrapper"},
                  Panel::create(
                      {
                          tag(
                              {"column2"},
                              Panel::create(
                                  {
                                      make_label("Action:"),
                                      a_opts,
                                  },
                                  ORIENT_VERTICAL)),
                          tag(
                              {"column2"},
                              Panel::create(
                                  {
                                      make_label("Target:"),
                                      t_opts,
                                  },
                                  ORIENT_VERTICAL)),
                      })),
              make_hbar(),
              tag(
                  {"section"},
                  Panel::create({b_cancel, b_commit})),
          },
          ORIENT_VERTICAL));
}
