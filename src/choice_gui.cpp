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

  children.push_front(make_hbar());
  children.push_front(make_label("Queue"));

  return tag(
      {"transparent", "choice-queue"},
      Panel::create(children, Panel::ORIENT_VERTICAL));
}

PanelPtr build_info(list<string> info) {
  list<ComponentPtr> children = utility::map<list<ComponentPtr>>(
      [](string v) {
        return tag({"label", "list-item"}, Button::create(v));
      },
      info);

  return tag({"choice-gui-info"}, Panel::create(children, Panel::ORIENT_VERTICAL));
}

PanelPtr st3::choice_gui(
    std::string title,
    std::list<string> options,
    option_generator f_opt,
    info_generator f_info,
    std::function<void(choice_gui_action, std::list<std::string>)> on_commit,
    RSG::Voidfun on_cancel,
    bool allow_queue) {
  // If this choice does not support queing, always use replace action
  bool hide_action = !allow_queue;

  auto make_section = [](string h) {
    return tag({"section", "default-panel"}, with_style({{"height", h}}, Panel::create()));
  };

  // Create sections
  PanelPtr cards_wrapper = make_section("30%");
  PanelPtr action_wrapper = make_section("20%");
  PanelPtr info_wrapper = make_section("30%");
  PanelPtr button_wrapper = make_section("15%");

  // Left panel section
  PanelPtr queue_wrapper = Panel::create();

  // Managed variables
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

    queue_wrapper->replace_children({build_queue(*queue)});
    b_commit->remove_state("disabled");
  };

  // Initialize queue wrapper
  queue_wrapper->replace_children({build_queue(*queue)});

  // Option cards
  list<ComponentPtr> cards = utility::map<list<ComponentPtr>>(
      [=](string v) -> ComponentPtr {
        ButtonPtr b = with_style({{"width", "22%"}, {"height", "75%"}}, f_opt(v));
        b->set_on_press([=](ButtonPtr self) { enque(v); });
        b->on_hover = [=]() {
          info_wrapper->clear_children();
          info_wrapper->add_child(build_info(f_info(v)));
        };
        return b;
      },
      options);

  cards_wrapper->replace_children(cards);

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

  action_options->set_style(
      {
          {"background-color", "00000000"},
          {"border-thickness", "0"},
      });

  action_wrapper->replace_children({action_options});

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

  // Button wrapper
  button_wrapper->replace_children({Button::create("Cancel", on_cancel), b_commit});

  // Main layout
  list<ComponentPtr> left_children = {
      cards_wrapper,
      action_options,
      info_wrapper,
      button_wrapper,
  };
  if (hide_action) left_children.remove(action_options);

  auto p = Panel::create(
      {
          tag({"h1"}, Button::create(title)),
          make_hbar(),
          tag(
              {"choice-wrapper"},
              Panel::create(
                  {
                      tag(
                          {"choice-left", "transparent"},
                          Panel::create(left_children, Panel::ORIENT_VERTICAL)),
                      tag(
                          {"choice-right", "transparent"},
                          queue_wrapper),
                  })),
      },
      Panel::ORIENT_VERTICAL);

  return p;
}
