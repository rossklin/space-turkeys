#include "choice_gui.hpp"

#include <functional>
#include <list>
#include <string>

#include "rsg/src/RskTypes.hpp"
#include "rsg/src/button.hpp"
#include "rsg/src/panel.hpp"
#include "style.hpp"
#include "utility.hpp"

using namespace std;
using namespace st3;
using namespace RSG;

PanelPtr build_queue(list<string> q) {
  // todo
}

PanelPtr build_info(list<string> info) {
  // todo
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

  return Panel::create(
      {
          make_label(title),
          make_hbar(),
          Panel::create(cards),
          make_hbar(),
          info_wrapper,
          make_hbar,
          queue_wrapper,
      },
      Panel::ORIENT_VERTICAL);
}
