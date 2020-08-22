#pragma once

#include <functional>
#include <string>

#include "rsg/src/RskTypes.hpp"
#include "types.hpp"

namespace st3 {
RSG::PanelPtr command_gui(
    std::string ship_class,
    std::string action,
    int policy,
    hm_t<std::string, int> num_available,
    int max_num,
    bool allow_combat,
    std::function<void(std::string ship_class, int policy, int num)> on_commit,
    RSG::Voidfun on_cancel,
    RSG::Voidfun on_delete);
};  // namespace st3
