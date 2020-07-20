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

// Todo: remove queue item on press
PanelPtr build_queue(list<string> q) {
  list<ComponentPtr> children = utility::map<list<ComponentPtr>>([](string v) { return Button::create(v); }, q);
  return Panel::create(children, Panel::ORIENT_VERTICAL);
}

PanelPtr build_info(list<string> info) {
  list<ComponentPtr> children = utility::map<list<ComponentPtr>>([](string v) { return make_label(v); }, info);
  return Panel::create(children, Panel::ORIENT_VERTICAL);
}

PanelPtr st3::choice_gui(
    std::string title,
    std::list<string> options,
    option_generator f_opt,
    info_generator f_info,
    std::function<void(choice_gui_action, std::list<std::string>)> on_commit,
    RSG::Voidfun on_cancel,
    bool allow_queue) {
  // Queue wrapper
  PanelPtr queue_wrapper = Panel::create();
  shared_ptr<list<string>> queue = make_shared<list<string>>();
  auto enque = [=](string v) {
    if (allow_queue) {
      queue->push_back(v);
    } else {
      *queue = {v};
    }

    queue_wrapper->clear_children();
    queue_wrapper->add_child(build_queue(*queue));
  };

  // Info wrapper
  PanelPtr info_wrapper = Panel::create();

  // Option cards
  list<ComponentPtr> cards = utility::map<list<ComponentPtr>>(
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

  // Todo: action

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
          make_hbar(),
          Panel::create({
              Button::create("Cancel", on_cancel),
              Button::create("Commit", [on_commit, queue]() {
                on_commit(CHOICEGUI_APPEND, *queue);
              }),
          }),
      },
      Panel::ORIENT_VERTICAL);
}
