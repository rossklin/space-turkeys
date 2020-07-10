#pragma once

#include <functional>
#include <list>
#include <string>

#include "rsg/src/panel.hpp"
#include "types.hpp"

namespace st3 {
typedef std::function<RSG::ButtonPtr(std::string)> option_generator;
typedef std::function<std::list<std::string>(std::string)> info_generator;
RSG::PanelPtr choice_gui(
    std::string title,
    std::list<string> options,
    option_generator f_opt,
    info_generator f_info,
    std::function<void(std::list<std::string>)> on_commit,
    RSG::Voidfun on_cancel,
    bool allow_queue);
};  // namespace st3
