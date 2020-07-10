#include "choice_gui.hpp"

#include <functional>
#include <list>
#include <string>

#include "rsg/src/RskTypes.hpp"
#include "rsg/src/button.hpp"
#include "rsg/src/component.hpp"
#include "rsg/src/panel.hpp"
#include "style.hpp"
#include "utility.hpp"

using namespace std;
using namespace st3;
using namespace RSG;

PanelPtr build_queue(list<string> q) {
  list<ComponentPtr> children = utility::map<list<string>, list<ComponentPtr>>([](string v) { return Button::create(v); }, q);
  return Panel::create(children, Panel::ORIENT_VERTICAL);
}

PanelPtr build_info(list<string> info) {
  list<ComponentPtr> children = utility::map<list<string>, list<ComponentPtr>>([](string v) { return make_label(v); }, info);
  return Panel::create(children, Panel::ORIENT_VERTICAL);
}

PanelPtr choice_gui(
    std::string title,
    std::list<string> options,
    option_generator f_opt,
    info_generator f_info,
    std::function<void(std::list<std::string>)> on_commit,
    RSG::Voidfun on_cancel,
    bool allow_queue) {
  // Queue wrapper
  PanelPtr queue_wrapper = Panel::create();
  list<string> queue;
  auto enque = [&](string v) {
    queue.push_back(v);
    queue_wrapper->clear_children();
    queue_wrapper->add_child(build_queue(queue));
  };

  // Info wrapper
  PanelPtr info_wrapper = Panel::create();

  // Option cards
  list<ComponentPtr> cards = utility::map<list<string>, list<ComponentPtr>>(
      [=](string v) -> ComponentPtr {
        ButtonPtr b = f_opt(v);
        b->set_on_press([=](ButtonPtr self) { enque(v); });
        b->on_hover = [=]() {
          info_wrapper->clear_children();
          info_wrapper->add_child(build_info(f_info(v)));
        };
        return b;
      },
      options);

  // Main layout
  return Panel::create(
      {
          make_label(title),
          make_hbar(),
          Panel::create(cards),
          make_hbar(),
          info_wrapper,
          make_hbar(),
          queue_wrapper,
      },
      Panel::ORIENT_VERTICAL);
}
