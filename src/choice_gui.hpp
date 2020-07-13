#pragma once

#include <functional>
#include <list>
#include <string>

#include "rsg/src/panel.hpp"
#include "types.hpp"

namespace st3 {

enum choice_gui_action {
  CHOICEGUI_APPEND,
  CHOICEGUI_PREPEND,
  CHOICEGUI_REPLACE
};

typedef std::function<RSG::ButtonPtr(std::string)> option_generator;
typedef std::function<std::list<std::string>(std::string)> info_generator;
RSG::PanelPtr choice_gui(
    std::string title,
    std::list<std::string> options,
    option_generator f_opt,
    info_generator f_info,
    std::function<void(choice_gui_action a, std::list<std::string> q)> on_commit,
    RSG::Voidfun on_cancel,
    bool allow_queue);
};  // namespace st3
