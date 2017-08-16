#include "choice_gui.h"
#include "military_gui.h"
#include "desktop.h"
#include "research.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace sfg;
using namespace interface;

// main window for military choice
Widget::Ptr military_gui() {
  set<string> options;
  for (auto v : ship::all_ship_classes()) options.insert(v);

  // military gui
  choice_gui::f_info_t f_info = [] (string v) -> choice_info {
    choice_info res;
    res.available = desktop -> get_research().can_build_ship(v, &res.requirements);
    res.progress = 0;
    return res;
  };

  choice_gui::f_result_t callback = [] (set<string> result, bool accepted) {
    cost::ship_allocation res;
    if (accepted) {
      for (auto v : result) res[v] = 1;
      desktop -> response.military = res;
    }
  };

  return choice_gui::Create(options, f_info, callback);
}
