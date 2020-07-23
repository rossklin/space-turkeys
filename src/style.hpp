#pragma once

#include <functional>
#include <string>

#include "rsg/src/RskTypes.hpp"

namespace st3 {
RSG::ButtonPtr make_label(std::string v);
RSG::ButtonPtr make_hbar();
RSG::ButtonPtr make_card(std::string v, int n, std::string h = "200px");

void generate_styles();
}  // namespace  st3
