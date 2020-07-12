#pragma once

#include <functional>
#include <list>
#include <set>
#include <string>

#include "rsg/src/RskTypes.hpp"
#include "types.hpp"

namespace st3 {
RSG::PanelPtr target_gui(
    sf::Vector2f position,
    hm_t<std::string, std::set<combid>> action_targets,
    // To create waypoint, set action to fleet::go_to and target to identifier::source_none
    std::function<void(std::string action, std::string target)> callback,
    RSG::Voidfun on_cancel);
};  // namespace st3
