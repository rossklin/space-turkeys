#pragma once

#include <functional>

#include "rsg/src/RskTypes.hpp"

namespace st3 {
RSG::ButtonPtr make_label(std::string v);
RSG::ButtonPtr make_hbar();
}  // namespace  st3
