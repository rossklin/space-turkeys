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

PanelPtr st3::target_gui(
    sf::Vector2f position,
    hm_t<std::string, std::set<combid>> action_targets,
    std::function<void(std::string action, std::string target)> callback,
    RSG::Voidfun on_cancel) {
  shared_ptr<string> sel_action = make_shared<string>("");
  shared_ptr<combid> sel_target = make_shared<combid>("");

  ButtonPtr b_cancel = Button::create("Cancel", on_cancel);
  ButtonPtr b_commit = Button::create("Commit", [sel_action, sel_target, callback]() { callback(*sel_action, *sel_target); });

  auto t_opts = ButtonOptions::create(list<string>{}, [sel_target](string k) { *sel_target = k; });
  t_opts->add_state("invisible");

  auto a_opts = ButtonOptions::create(
      utility::range_init<list<string>>(utility::hm_keys(action_targets)),
      [=](string k) {
        if (*sel_action == k) return;

        *sel_action = k;
        *sel_target = "";

        map<string, ButtonPtr> buf;
        for (auto v : action_targets.at(k)) buf[k] = Button::create(k);
        t_opts->set_children(buf);
        t_opts->remove_state("invisible");
      });

  return styled<Panel, list<ComponentPtr>, Panel::orientation>(
      {{"position", "fixed"}, {"top", to_string(position.y)}, {"left", to_string(position.x)}},
      {
          Panel::create({a_opts, t_opts}),
          Panel::create({b_cancel, b_commit}),
      },
      Panel::ORIENT_VERTICAL);
}
