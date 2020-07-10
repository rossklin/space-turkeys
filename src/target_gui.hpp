#pragma once

#include <functional>
#include <list>
#include <string>

#include "rsg/src/RskTypes.hpp"

namespace st3 {
RSG::PanelPtr target_gui(sf::Vector2f position, std::list<std::string> options, std::function<void(std::string)> callback);
};  // namespace st3
