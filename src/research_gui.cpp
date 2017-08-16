#include "research_gui.h"
#include "desktop.h"
#include "research.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace sfg;
using namespace interface;

// main window for research choice
Widget::Ptr research_gui() {
  hm_t<string, research::tech> map;
  set<string> options;
  for (auto &f : research::data::table()) {
    map[f.first] = f.second;
    options.insert(f.first);
  }
  
  for (auto &t : desktop -> get_research().tech_map) map[t.first] = t.second;

  // development gui
  choice_gui::f_info_t f_info = [map] (string v) -> choice_info {
    choice_info res;
    research::tech n = map[v];

    // requirements
    res.requirements = desktop -> get_research().list_tech_requirements(v);
    if (desktop -> get_research().researched().count(v)) res.requirements.push_back("Already researched");

    res.available = res.requirements.empty();
    res.progress = n.progress;

    // info
    for (auto b : n.sector_boost) res.info.push_back(b.first + " + " + to_string((int)(100 * (b.second - 1))) + "%");
    for (auto su : n.ship_upgrades) res.info.push_back(su.first + " gains: " + boost::algorithm::join(su.second, ", "));

    return res;
  };

  choice_gui::f_result_t callback = [] (set<string> result, bool accepted) {
    if (accepted) {
      if (result.size() == 1) {
	desktop -> response.research = *result.begin();
      } else {
	throw runtime_error("research_gui: callback: bad result size!");
      }
    }
  };

  return choice_gui::Create("Research", true, options, f_info, callback);
}
