#include "choice_gui.hpp"

#include <functional>
#include <list>
#include <string>

#include "rsg/src/RskTypes.hpp"
#include "rsg/src/button.hpp"
#include "rsg/src/button_options.hpp"
#include "rsg/src/component.hpp"
#include "rsg/src/panel.hpp"
#include "rsg/src/utility.hpp"
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
  list<ComponentPtr> children = utility::map<list<ComponentPtr>>(
      [](string v) {
        auto p = make_label(v);
        p->set_style({
            {"font-size", "11"},
            {"padding-top", "0"},
            {"padding-bottom", "0"},
            {"margin-top", "5"},
            {"margin-bottom", "5"},
        });
        return p;
      },
      info);
  return Panel::create(children, Panel::ORIENT_VERTICAL);
}

PanelPtr st3::choice_gui(
    std::string title,
    std::list<string> options,
    option_generator f_opt,
    info_generator f_info,
    std::function<void(choice_gui_action, std::list<std::string>)> on_commit,
    RSG::Voidfun on_cancel,
    bool allow_queue,
    bool hide_action) {
  // Queue wrapper
  PanelPtr queue_wrapper = Panel::create();
  shared_ptr<list<string>> queue = make_shared<list<string>>();
  shared_ptr<choice_gui_action> action = make_shared<choice_gui_action>(CHOICEGUI_REPLACE);

  ButtonPtr b_commit = Button::create("Commit", [on_commit, queue, action]() {
    on_commit(*action, *queue);
  });
  b_commit->add_state("disabled");

  auto enque = [=](string v) {
    if (allow_queue) {
      queue->push_back(v);
    } else {
      *queue = {v};
    }

    queue_wrapper->clear_children();
    queue_wrapper->add_child(build_queue(*queue));
    b_commit->remove_state("disabled");
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

  // Action
  ButtonOptionsPtr action_options = ButtonOptions::create(
      {"Append", "Prepend", "Replace"},
      [action](string k) {
        if (k == "Append") {
          *action = CHOICEGUI_APPEND;
        } else if (k == "Prepend") {
          *action = CHOICEGUI_PREPEND;
        } else if (k == "Replace") {
          *action = CHOICEGUI_REPLACE;
        }
      });

  switch (*action) {
    case CHOICEGUI_APPEND:
      action_options->select("Append");
      break;

    case CHOICEGUI_PREPEND:
      action_options->select("Prepend");
      break;

    case CHOICEGUI_REPLACE:
      action_options->select("Replace");
      break;

    default:
      break;
  }

  // Main layout
  list<ComponentPtr> children = {
      make_label(title),
      make_hbar(),
      Panel::create(cards),
      make_hbar(),
  };

  if (!hide_action) {
    children.push_back(action_options);
    children.push_back(make_hbar());
  }

  list<ComponentPtr> children2 = {
      info_wrapper,
      make_hbar(),
      queue_wrapper,
      make_hbar(),
      Panel::create({
          Button::create("Cancel", on_cancel),
          b_commit,
      }),
  };

  children.insert(children.end(), children2.begin(), children2.end());

  PanelPtr p = styled<Panel, list<ComponentPtr>, Panel::orientation>(
      {},
      children,
      Panel::ORIENT_VERTICAL);

  // Prevent click propagation
  p->on_click = [](ComponentPtr self, sf::Event e) {};
  return p;
}
