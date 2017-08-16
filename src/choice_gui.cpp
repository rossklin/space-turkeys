#include <set>
#include <boost/algorithm/string/join.hpp>
#include <SFGUI/SFGUI.hpp>
#include <SFGUI/Widgets.hpp>
#include <SFGUI/Selector.hpp>

#include "desktop.h"
#include "choice_gui.h"
#include "graphics.h"

using namespace std;
using namespace st3;
using namespace sfg;
using namespace interface;

const string choice_gui::sfg_id = "choice_gui";
const string choice_gui::class_selected = "choice-selected";
const string choice_gui::class_normal = "choice-normal";

choice_gui::Ptr choice_gui::Create(string title, bool unique, set<string> options, f_info_t info, f_result_t callback) {
  Ptr buf(new choice_gui(title, unique, options, info, callback));
  buf -> Add(buf -> frame);
  buf -> SetId(sfg_id);
  return buf;
}

choice_gui::choice_gui(string title, bool _unique, set<string> _options, f_info_t _info, f_result_t _callback) : Bin() {
  unique = _unique;
  options = _options;
  f_info = _info;
  callback = _callback;
  width = 600;

  // do "css" stuff
  auto hierarchy = Selector::HierarchyType::ROOT;
  Selector::Ptr sel = Selector::Create( "Button", "", class_selected, "", hierarchy, 0);
  string sel_string = sel -> BuildString();
  desktop -> SetProperty(sel_string, "BorderWidth", 3);
  desktop -> SetProperty(sel_string, "BorderColor", sf::Color::White);
  sel = Selector::Create( "Button", "", class_normal, "", hierarchy, 0);
  desktop -> SetProperty(sel_string, "BorderWidth", 1);
  desktop -> SetProperty(sel_string, "BorderColor", sf::Color::Black);

  frame = Frame::Create(title);
  layout = Box::Create(Box::Orientation::VERTICAL, 5);
  frame -> Add(layout);
    
  setup();
}

const string& choice_gui::GetName() const {
  return sfg_id;
}

sf::Vector2f choice_gui::CalculateRequisition() {
  return frame -> GetRequisition();
}

void choice_gui::update_selected() {
  for (auto x : buttons) {
    string class_string = class_normal;
    if (selected.count(x.first)) class_string = class_selected;
    x.second -> SetClass(class_string);
  }
}

void choice_gui::setup() {
  info_area = Box::Create(Box::Orientation::VERTICAL, 10);

  auto build = [this] (string v) -> Widget::Ptr {
    // add image button
    auto b = Button::Create();
    buttons[v] = b;
    choice_info info = f_info(v);
    b -> SetImage(Image::Create(graphics::selector_card(v, info.available, info.progress)));

    // set on_select callback
    desktop -> bind_ppc(b, [this, v, info] () {
	if (info.available) {
	  if (unique) selected.clear();
	  if (selected.count(v)) {
	    selected.erase(v);
	  } else {
	    selected.insert(v);
	  }
	  
	  update_selected();
	}

	// update info area
	info_area -> RemoveAll();
	Frame::Ptr about = Frame::Create(v);
	for (auto q : info.info) about -> Add(Label::Create(q));
	info_area -> Pack(about);

	Frame::Ptr req = Frame::Create("Requirements");
	for (auto q : info.requirements) req -> Add(Label::Create(q));
	info_area -> Pack(req);
      });
    
    return b;
  };

  // options go in a scrolled window
  Box::Ptr window_box = Box::Create(Box::Orientation::HORIZONTAL, 5);
  for (auto v : options) window_box -> Pack(build(v));
  update_selected();
  layout -> Pack(graphics::wrap_in_scroll(window_box, true, width));

  // info area
  layout -> Pack(info_area);

  // buttons for ok/cancel
  Box::Ptr confirm_box = Box::Create(Box::Orientation::HORIZONTAL, 5);
  auto b_cancel = Button::Create("Cancel");
  desktop -> bind_ppc(b_cancel, [this] () {
      callback(selected, false);
      desktop -> clear_qw();
    });
  confirm_box -> Pack(b_cancel);
  
  auto b_accept = Button::Create("Accept");
  desktop -> bind_ppc(b_accept, [this] () {
      if (selected.size()) {
	callback(selected, true);
	desktop -> clear_qw();
      }
    });
  confirm_box -> Pack(b_accept);

  layout -> Pack(confirm_box);
}

// GOVERNOR
Widget::Ptr interface::governor_gui(list<solar::ptr> solars) {
  set<string> options;
  for (auto v : keywords::sector) options.insert(v);

  choice_gui::f_info_t f_info = [] (string v) -> choice_info {
    choice_info res;
    res.info.push_back("Governor focused on " + v);
    res.available = true;
    return res;
  };

  choice_gui::f_result_t callback = [solars] (set<string> selected, bool accepted) {
    if (accepted) {
      if (selected.size() == 1) {
	string gov = *selected.begin();
	for (auto s : solars) s -> choice_data.governor = gov;
      } else {
	throw runtime_error("governor_gui: invalid selection size!");
      }
    }
  };

  return choice_gui::Create("Governor", true, options, f_info, callback);
}

// RESEARCH
Widget::Ptr interface::research_gui() {
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
    research::tech n = map.at(v);

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

// MILITARY
Widget::Ptr interface::military_gui() {
  set<string> options;
  for (auto v : ship::all_classes()) options.insert(v);

  // military gui
  choice_gui::f_info_t f_info = [] (string v) -> choice_info {
    choice_info res;
    ship_stats s = ship::table().at(v);
    res.available = s.depends_tech.empty() || desktop -> get_research().researched().count(s.depends_tech);
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

  return choice_gui::Create("Military", false, options, f_info, callback);
}
