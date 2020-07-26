#include "command_gui.hpp"

#include <functional>
#include <list>
#include <string>

#include "fleet.hpp"
#include "rsg/src/RskTypes.hpp"
#include "rsg/src/button.hpp"
#include "rsg/src/button_options.hpp"
#include "rsg/src/panel.hpp"
#include "rsg/src/slider.hpp"
#include "rsg/src/utility.hpp"
#include "style.hpp"
#include "utility.hpp"

using namespace std;
using namespace st3;
using namespace RSG;

PanelPtr st3::command_gui(
    string original_ship_class,
    string action,
    int original_policy,
    hm_t<string, int> num_available,
    int max_num,
    bool allow_combat,
    function<void(string ship_class, int policy, int num)> on_commit,
    Voidfun on_cancel) {
  // Apply max num
  num_available = utility::hm_map<string, int, int>([max_num](string k, int v) { return min(v, max_num); }, num_available);

  // Policy panel
  hm_t<string, int> policies = {
      {"Maintain course", fleet::policy_maintain_course},
      {"Aggressive", fleet::policy_aggressive},
      {"Evasive", fleet::policy_evasive},
  };

  shared_ptr<int> policy = make_shared<int>(original_policy);
  PanelPtr policy_panel = tag(
      {"section"},
      Panel::create(
          {
              spaced_ul(make_label("Enemy reaction policy")),
              ButtonOptions::create(
                  utility::range_init<list<string>>(utility::hm_keys(policies)),
                  [policies, policy](string k) {
                    *policy = policies.at(k);
                  }),
          },
          Panel::ORIENT_VERTICAL));

  // Slider
  shared_ptr<string> ship_class = make_shared<string>(original_ship_class);
  shared_ptr<int> num = make_shared<int>(num_available[original_ship_class]);
  ButtonPtr slider_label = make_label("Ships to send: " + to_string(*num));

  auto build_slider = [num, slider_label, num_available, ship_class]() {
    Slider::handler_t slider_handler = [num, slider_label](SliderPtr self, float new_value, float new_ratio) {
      *num = round(new_value);
      slider_label->set_label("Ships to send: " + to_string(*num));
    };

    return Slider::create(0, num_available.at(*ship_class), num_available.at(*ship_class), slider_handler);
  };

  PanelPtr slider_panel = tag({"section"}, Panel::create({slider_label, build_slider()}, Panel::ORIENT_VERTICAL));

  ButtonPtr title = make_label("Assign " + *ship_class + " ships for '" + action + "' command");

  // Ship class panel
  PanelPtr sc_panel = tag(
      {"section"},
      Panel::create(
          {
              spaced_ul(make_label("Select ship class to send")),

              ButtonOptions::create(
                  utility::range_init<list<string>>(utility::hm_keys(num_available)),
                  [ship_class, action, slider_panel, slider_label, build_slider, title](string sc) {
                    *ship_class = sc;
                    title->set_label("Assign " + *ship_class + " ships for '" + action + "' command");
                    slider_panel->replace_children({slider_label, build_slider()});
                  }),
          }));

  // Button panel
  ButtonPtr b_cancel = Button::create("Cancel", [on_cancel](ButtonPtr b) { on_cancel(); });
  ButtonPtr b_commit = Button::create("Commit", [on_commit, ship_class, policy, num](ButtonPtr b) { on_commit(*ship_class, *policy, *num); });
  PanelPtr button_panel = tag({"section"}, Panel::create({b_cancel, b_commit}));

  return tag(
      {"command-gui"},
      Panel::create(
          {
              title,
              make_hbar(),
              sc_panel,
              make_hbar(),
              policy_panel,
              make_hbar(),
              slider_panel,
              make_hbar(),
              button_panel,
          },
          Panel::ORIENT_VERTICAL));
}
