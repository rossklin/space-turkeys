#include "command_gui.hpp"

#include <functional>
#include <list>
#include <string>

#include "fleet.hpp"
#include "rsg/src/RskTypes.hpp"
#include "rsg/src/button.hpp"
#include "rsg/src/panel.hpp"
#include "rsg/src/slider.hpp"
#include "rsg/src/utility.hpp"
#include "style.hpp"
#include "utility.hpp"

using namespace std;
using namespace st3;
using namespace RSG;

PanelPtr command_gui(
    string ship_class,
    string action,
    int policy,
    int num_available,
    bool allow_combat,
    Voidfun on_cancel,
    function<void(int, int)> on_commit) {
  // Policy panel
  hm_t<int, string> policies = {
      {fleet::policy_maintain_course, "Maintain course"},
      {fleet::policy_aggressive, "Aggressive"},
      {fleet::policy_evasive, "Evasive"},
  };
  list<ComponentPtr> policy_buttons;

  auto policy_setter = [&policy, &policy_buttons](int x) {
    return [&policy, &policy_buttons, x](ButtonPtr s) {
      policy = x;
      for (auto b : policy_buttons) b->remove_state("selected");
      s->add_state("selected");
    };
  };

  for (auto x : policies) {
    ButtonPtr b = Button::create(x.second, policy_setter(x.first));
    if (x.first == policy) b->add_state("selected");
    policy_buttons.push_back(b);
  }

  PanelPtr policy_panel = styled<Panel, list<ComponentPtr>>(
      {},
      {
          make_label("Policy:"),
          Panel::create(policy_buttons),
      });

  // Slider
  int num = num_available;
  ButtonPtr slider_label = make_label("Ships to send: " + to_string(num));

  Slider::handler_t slider_handler = [&num, slider_label](SliderPtr self, float new_value, float new_ratio) {
    num = new_value;
    slider_label->set_label("Ships to send: " + to_string(num));
  };

  SliderPtr slider = Slider::create(0, num_available, num_available, slider_handler);

  PanelPtr slider_panel = Panel::create({slider_label, slider}, Panel::ORIENT_VERTICAL);

  // Button panel
  ButtonPtr b_cancel = Button::create("Cancel", [on_cancel](ButtonPtr b) { on_cancel(); });
  ButtonPtr b_commit = Button::create("Commit", [on_commit, &policy, &num](ButtonPtr b) { on_commit(policy, num); });
  PanelPtr button_panel = Panel::create({b_cancel, b_commit});

  return styled<Panel, list<ComponentPtr>, Panel::orientation>(
      {},
      {
          make_label("Assign ships for '" + action + "' command"),
          make_hbar(),
          policy_panel,
          make_hbar(),
          slider_panel,
          make_hbar(),
          button_panel,
      },
      Panel::ORIENT_VERTICAL);
}
