#include "choice_gui.hpp"

#include <functional>
#include <list>
#include <string>

#include "RskTypes.hpp"
#include "panel.hpp"

using namespace std;
using namespace st3;
using namespace RSG;

RSG::PanelPtr choice_gui(
    std::string title,
    std::list<string> options,
    option_generator f_opt,
    std::function<void(std::list<std::string>)> on_commit,
    RSG::Voidfun on_cancel,
    bool allow_queue) {
  return Panel::create({

  });
}
