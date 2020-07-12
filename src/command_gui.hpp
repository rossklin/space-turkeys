#pragma once

#include <functional>
#include <string>

#include "rsg/src/RskTypes.hpp"

namespace st3 {
RSG::PanelPtr command_gui(
    std::string ship_class,
    std::string action,
    int policy,
    int num_available,
    bool allow_combat,
    std::function<void(int, int)> on_commit,
    RSG::Voidfun on_cancel);
};  // namespace st3
