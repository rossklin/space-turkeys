#include <set>
#include <boost/algorithm/string/join.hpp>
#include <SFGUI/SFGUI.hpp>
#include <SFGUI/Widgets.hpp>

#include "desktop.h"
#include "choice_gui.h"
#include "graphics.h"
#include "client_game.h"

using namespace std;
using namespace st3;
using namespace sfg;
using namespace interface;

const string choice_gui::sfg_id = "choice_gui";
const string choice_gui::class_selected = "choice-selected";
const string choice_gui::class_normal = "choice-normal";

bool test_unique_choice(hm_t<string, bool> options, string &res) {
  int n = 0;

  for (auto x : options) {
    n += x.second;
    if (x.second) res = x.first;
  }

  return n == 1;
}

choice_info::choice_info() {
  available = false;
  progress = 0;
}

choice_gui::Ptr choice_gui::Create(std::string title,
				   std::string help_text,
				   bool unique,
				   hm_t<std::string, bool> options,
				   f_info_t info,
				   f_result_t callback) {
  Ptr buf(new choice_gui(title, help_text, unique, options, info, callback));
  buf -> Add(buf -> frame);
  buf -> SetId(sfg_id);
  return buf;
}

hm_t<string, int> choice_gui::calc_faclev() {
  hm_t<string, int> res;

  for (auto x : solar::facility_table()) {
    string v = x.first;
    int faclev = 0;
    for (auto s : desktop -> g -> get_all<solar>()) {
      if (s -> owned) faclev = max(faclev, s -> facility_access(v) -> level);
    }
    res[v] = faclev;
  }
  
  return res;
}

choice_gui::choice_gui(std::string title,
		       std::string help_text,
		       bool _unique,
		       hm_t<std::string, bool> _options,
		       f_info_t _info,
		       f_result_t _callback) : Window(Window::Style::BACKGROUND) {
  unique = _unique;
  options = _options;
  f_info = _info;
  callback = _callback;
  width = 600;

  frame = Frame::Create(title);
  layout = Box::Create(Box::Orientation::VERTICAL, 5);

  // build a choice button
  auto build = [this] (string v) -> Widget::Ptr {
    // add image button
    auto b = Button::Create();
    buttons[v] = b;
    choice_info info = f_info(v);
    b -> SetImage(Image::Create(graphics::selector_card(v, info.available, info.progress)));
    string class_string = options[v] ? class_selected : class_normal;
    b -> SetClass(class_string);

    // set on_select callback
    desktop -> bind_ppc(b, [this, v, info] () {
	if (info.available) {
	  if (unique) for (auto &x : options) x.second = false;
	  options[v] = !options[v];
	  update_selected();
	}

	// update info area
	info_area -> RemoveAll();
	Frame::Ptr about = Frame::Create(v);
	Box::Ptr buf = Box::Create(Box::Orientation::VERTICAL);
	for (auto q : info.info) buf -> Pack(Label::Create(q));
	about -> Add(buf);
	info_area -> Pack(about);

	Frame::Ptr req = Frame::Create("Requirements");
	buf = Box::Create(Box::Orientation::VERTICAL);
	for (auto q : info.requirements) buf -> Pack(Label::Create(q));
	req -> Add(buf);
	info_area -> Pack(req);

	Widget::RefreshAll();
      });
    
    return b;
  };

  // help text area
  Box::Ptr help_area = Box::Create();
  help_area -> Pack(Label::Create(help_text));
  layout -> Pack(help_area);

  // sort options
  vector<string> opt_buf;
  for (auto x : options) opt_buf.push_back(x.first);
  sort(opt_buf.begin(), opt_buf.end(), [this] (string a, string b) -> bool {
      return f_info(a).available > f_info(b).available;
    });

  // options go in a scrolled window
  Box::Ptr window_box = Box::Create(Box::Orientation::HORIZONTAL, 5);
  for (auto v : opt_buf) window_box -> Pack(build(v));
  update_selected();
  layout -> Pack(graphics::wrap_in_scroll(window_box, true, width));

  // info area
  info_area = Box::Create(Box::Orientation::VERTICAL, 10);
  layout -> Pack(info_area);

  // buttons for ok/cancel
  Box::Ptr confirm_box = Box::Create(Box::Orientation::HORIZONTAL, 5);
  auto b_cancel = Button::Create("Cancel");
  desktop -> bind_ppc(b_cancel, [this] () {
      callback(options, false);
      desktop -> clear_qw();
    });
  confirm_box -> Pack(b_cancel);
  
  auto b_accept = Button::Create("Accept");
  desktop -> bind_ppc(b_accept, [this] () {
      int n = 0;
      for (auto x : options) n += x.second;
      
      if (n == 1 || !unique) {
	callback(options, true);
	desktop -> clear_qw();
      }
    });
  confirm_box -> Pack(b_accept);

  layout -> Pack(confirm_box);
  
  frame -> Add(layout);
}

const string& choice_gui::GetName() const {
  return sfg_id;
}

void choice_gui::update_selected() {
  for (auto x : buttons) {
    string class_string = class_normal;
    if (options[x.first]) class_string = class_selected;
    x.second -> SetClass(class_string);
  }
}

// GOVERNOR
Widget::Ptr interface::governor_gui(list<solar::ptr> solars) {
  hm_t<string, bool> options;
  for (auto v : keywords::governor) options[v] = false;

  // test if current governor is consistent
  string gov = solars.front() -> choice_data.governor;
  if (gov.size()) {
    bool test = true;
    for (auto s : solars) test &= s -> choice_data.governor == gov;
    if (test) options[gov] = true;
  }

  choice_gui::f_info_t f_info = [] (string v) -> choice_info {
    choice_info res;
    res.info.push_back("Governor focused on " + v);
    res.available = true;
    return res;
  };

  choice_gui::f_result_t callback = [solars] (hm_t<string, bool> selected, bool accepted) {
    if (accepted) {
      string test;      
      if (test_unique_choice(selected, test)) {
	string gov = test;
	for (auto s : solars) s -> choice_data.governor = gov;
      } else {
	throw runtime_error("governor_gui: invalid selection size!");
      }
    }
  };

  return choice_gui::Create("Governor", "Chose a governor for the selected solars", true, options, f_info, callback);
}

// RESEARCH
Widget::Ptr interface::research_gui() {
  hm_t<string, research::tech> map;
  hm_t<string, bool> options;
  
  for (auto &f : research::data::table()) {
    map[f.first] = f.second;
    options[f.first] = false;
  }
  
  for (auto &t : desktop -> get_research().tech_map) map[t.first] = t.second;
  desktop -> access_research() -> facility_level = choice_gui::calc_faclev();
  
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

  choice_gui::f_result_t callback = [] (hm_t<string, bool> result, bool accepted) {
    if (accepted) {
      string test;
      if (test_unique_choice(result, test)) {
	desktop -> response.research = test;
      } else {
	throw runtime_error("research_gui: callback: bad result size!");
      }
    }
  };

  string suggest = desktop -> response.research;
  if (suggest.size() && f_info(suggest).available) {
    options[suggest] = true;
  }

  return choice_gui::Create("Research", "Chose a tech to research", true, options, f_info, callback);
}

// MILITARY
Widget::Ptr interface::military_gui() {
  hm_t<string, bool> options;
  for (auto v : ship::all_classes()) options[v] = desktop -> response.military[v] > 0;

  // get highest shipyard level
  int faclev = choice_gui::calc_faclev()["shipyard"];

  // military gui
  choice_gui::f_info_t f_info = [faclev] (string v) -> choice_info {
    choice_info res;
    ship_stats s = ship::table().at(v);
    bool tech_ok = s.depends_tech.empty() || desktop -> get_research().researched().count(s.depends_tech);
    if (!tech_ok) res.requirements.push_back("Research " + s.depends_tech);
    if (s.depends_facility_level > faclev) res.requirements.push_back("Shipyard level " + to_string(s.depends_facility_level));
    res.available = res.requirements.empty();
    res.progress = 0;
    return res;
  };

  choice_gui::f_result_t callback = [] (hm_t<string, bool> result, bool accepted) {
    cost::ship_allocation res;
    if (accepted) {
      for (auto v : result) res[v.first] = v.second;
      desktop -> response.military = res;
    }
  };

  return choice_gui::Create("Military", "Chose which ships should be produced in your empire", false, options, f_info, callback);
}
