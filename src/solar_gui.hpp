#pragma once

#include <functional>
#include <list>
#include <string>

#include "RskTypes.hpp"
#include "research.hpp"
#include "types.hpp"

namespace st3 {

RSG::PanelPtr solar_gui(
    solar_ptr s,
    research::data r,
    RSG::Voidfun on_cancel,
    std::function<void(std::string ship_to_build)> on_commit);
};  // namespace st3