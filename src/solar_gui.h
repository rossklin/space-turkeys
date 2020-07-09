#pragma once

#include <functional>
#include <list>
#include <string>

#include "RskTypes.hpp"
#include "research.h"
#include "types.h"

namespace st3 {
namespace interface {

RSG::PanelPtr solar_gui(
    solar_ptr s,
    research::data r,
    RSG::Voidfun on_cancel,
    std::function<void(std::list<std::string>, std::list<std::string>)> on_commit);
};  // namespace interface
};  // namespace st3