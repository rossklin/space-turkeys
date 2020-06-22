#include "choice_gui.h"

#include <SFGUI/SFGUI.hpp>
#include <SFGUI/Widgets.hpp>
#include <boost/algorithm/string/join.hpp>
#include <set>

#include "client_game.h"
#include "desktop.h"
#include "graphics.h"

using namespace std;
using namespace st3;
using namespace sfg;
using namespace interface;

const string solar_gui::sfg_id = "solar_gui";

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
  buf->Add(buf->frame);
  buf->SetId(sfg_id);
  return buf;
}

hm_t<string, int> choice_gui::calc_faclev() {
  hm_t<string, int> res;

  for (auto v : keywords::development) {
    int faclev = 0;
    for (auto s : desktop->g->get_all<solar>()) {
      if (s->owned) faclev = max(faclev, (int)s->effective_level(v));
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
  auto build = [this](string v) -> Widget::Ptr {
    // add image button
    auto b = Button::Create();
    buttons[v] = b;
    choice_info info = f_info(v);
    b->SetImage(Image::Create(graphics::selector_card(v, info.available, info.progress)));
    string class_string = options[v] ? class_selected : class_normal;
    b->SetClass(class_string);

    // set on_select callback
    desktop->bind_ppc(b, [this, v, info]() {
      if (info.available) {
        if (unique)
          for (auto &x : options) x.second = false;
        options[v] = !options[v];
        update_selected();
      }

      // update info area
      info_area->RemoveAll();
      Frame::Ptr about = Frame::Create(v);
      Box::Ptr buf = Box::Create(Box::Orientation::VERTICAL);
      for (auto q : info.info) buf->Pack(Label::Create(q));
      about->Add(buf);
      info_area->Pack(about);

      Frame::Ptr req = Frame::Create("Requirements");
      buf = Box::Create(Box::Orientation::VERTICAL);
      for (auto q : info.requirements) buf->Pack(Label::Create(q));
      req->Add(buf);
      info_area->Pack(req);

      Widget::RefreshAll();
    });

    return b;
  };

  // help text area
  Box::Ptr help_area = Box::Create();
  help_area->Pack(Label::Create(help_text));
  layout->Pack(help_area);

  // sort options
  vector<string> opt_buf;
  for (auto x : options) opt_buf.push_back(x.first);
  sort(opt_buf.begin(), opt_buf.end(), [this](string a, string b) -> bool {
    if (f_info(a).progress < 0) return true;
    if (f_info(b).progress < 0) return false;
    return f_info(a).available > f_info(b).available;
  });

  // options go in a scrolled window
  Box::Ptr window_box = Box::Create(Box::Orientation::HORIZONTAL, 5);
  for (auto v : opt_buf) window_box->Pack(build(v));
  update_selected();
  layout->Pack(graphics::wrap_in_scroll(window_box, true, width));

  // info area
  info_area = Box::Create(Box::Orientation::VERTICAL, 10);
  layout->Pack(info_area);

  // buttons for ok/cancel
  Box::Ptr confirm_box = Box::Create(Box::Orientation::HORIZONTAL, 5);
  auto b_cancel = Button::Create("Cancel");
  desktop->bind_ppc(b_cancel, [this]() {
    callback(options, false);
    desktop->clear_qw();
  });
  confirm_box->Pack(b_cancel);

  auto b_accept = Button::Create("Accept");
  desktop->bind_ppc(b_accept, [this]() {
    int n = 0;
    for (auto x : options) n += x.second;

    if (n == 1 || !unique) {
      callback(options, true);
      desktop->clear_qw();
    }
  });
  confirm_box->Pack(b_accept);

  layout->Pack(confirm_box);

  frame->Add(layout);
}

const string &choice_gui::GetName() const {
  return sfg_id;
}

void choice_gui::update_selected() {
  for (auto x : buttons) {
    string class_string = class_normal;
    if (options[x.first]) class_string = class_selected;
    x.second->SetClass(class_string);
  }
}

// GOVERNOR
Widget::Ptr interface::governor_gui(list<solar::ptr> solars) {
  hm_t<string, bool> options;
  for (auto v : keywords::development) options[v] = false;

  // test if current governor is consistent
  string current = solars.front()->choice_data.devstring();
  bool test = true;
  for (auto s : solars)
    if (s->choice_data.devstring() != current) current = "mixed";

  choice_gui::f_info_t f_info = [](string v) -> choice_info {
    choice_info res;
    res.info.push_back("Develop " + v);
    res.available = true;
    return res;
  };

  choice_gui::f_result_t callback = [solars](hm_t<string, bool> selected, bool accepted) {
    if (accepted) {
      string test;
      if (test_unique_choice(selected, test)) {
        for (auto s : solars) {
          s->choice_data.building_queue.clear();
          s->choice_data.building_queue.push_back(test);
        }
      } else {
        throw classified_error("governor_gui: invalid selection size!");
      }
    }
  };

  return choice_gui::Create("Development", "Chose what to develop in the selected solars", true, options, f_info, callback);
}

// RESEARCH
Widget::Ptr interface::research_gui() {
  hm_t<string, research::tech> map;
  hm_t<string, bool> options;
  hm_t<string, set<string> > leads_to;

  for (auto &f : research::data::table()) {
    map[f.first] = f.second;
    options[f.first] = false;
    for (auto v : f.second.depends_techs) leads_to[v].insert(f.first);
  }

  for (auto &t : desktop->get_research().tech_map) map[t.first] = t.second;
  desktop->access_research()->facility_level = choice_gui::calc_faclev();

  // development gui
  choice_gui::f_info_t f_info = [map, leads_to](string v) -> choice_info {
    choice_info res;
    research::tech n = map.at(v);

    // requirements
    res.requirements = desktop->get_research().list_tech_requirements(v);

    res.available = res.requirements.empty();

    if (desktop->get_research().researched().count(v)) {
      res.available = false;
      res.progress = -1;
    } else {
      res.progress = n.progress / n.cost_time;
    }

    // info
    for (auto b : n.solar_modifier) res.info.push_back(b.first + " + " + to_string(b.second));
    for (auto su : n.ship_upgrades) res.info.push_back(su.first + " gains: " + boost::algorithm::join(su.second, ", "));
    if (leads_to.count(v)) res.info.push_back("Leads to: " + boost::algorithm::join(leads_to.at(v), ", "));

    return res;
  };

  choice_gui::f_result_t callback = [](hm_t<string, bool> result, bool accepted) {
    if (accepted) {
      string test;
      if (test_unique_choice(result, test)) {
        desktop->response.research = test;
      } else {
        throw classified_error("research_gui: callback: bad result size!");
      }
    }
  };

  string suggest = desktop->response.research;
  if (suggest.size() && f_info(suggest).available) {
    options[suggest] = true;
  }

  return choice_gui::Create("Research", "Chose a tech to research", true, options, f_info, callback);
}

// MILITARY
Widget::Ptr interface::military_gui(list<solar::ptr> solars) {
  hm_t<string, bool> options;
  for (auto v : ship::all_classes()) options[v] = false;

  // get highest shipyard level
  int faclev = 0;
  for (auto s : solars) faclev = max(faclev, (int)s->effective_level(keywords::key_shipyard));

  // military gui
  choice_gui::f_info_t f_info = [faclev](string v) -> choice_info {
    choice_info res;
    ship_stats s = ship::table().at(v);
    bool tech_ok = s.depends_tech.empty() || desktop->get_research().researched().count(s.depends_tech);
    if (!tech_ok) res.requirements.push_back("Research " + s.depends_tech);
    if (s.depends_facility_level > faclev) res.requirements.push_back("Shipyard level " + to_string(s.depends_facility_level));
    res.available = res.requirements.empty();
    res.progress = 0;
    return res;
  };

  choice_gui::f_result_t callback = [solars](hm_t<string, bool> result, bool accepted) {
    if (accepted) {
      string test;
      for (auto v : result) {
        if (!v.second) continue;

        string k = v.first;
        for (auto s : solars) {
          ship_stats ss = ship::table().at(k);
          if (ss.depends_facility_level > s->effective_level(keywords::key_shipyard)) continue;

          s->choice_data.ship_queue.clear();
          s->choice_data.ship_queue.push_back(v.first);
        }
      }
    }
  };

  return choice_gui::Create("Military", "Chose which ships should be produced in your empire", false, options, f_info, callback);
}

solar_gui::Ptr solar_gui::Create(solar::ptr s) {
  Ptr buf(new solar_gui(s));
  buf->Add(buf->wrapper);
  buf->SetId(sfg_id);
  return buf;
}

solar_gui::solar_gui(solar::ptr s) : Window(Window::Style::BACKGROUND), sol(s) {
  wrapper = Box::Create(Box::Orientation::VERTICAL, 5);
  auto layout = Box::Create(Box::Orientation::HORIZONTAL, 5);
  auto frame_left = Frame::Create("Available buildings");
  auto frame_left_q = Frame::Create("Building queue");
  auto frame_right = Frame::Create("Available ships");
  auto frame_right_q = Frame::Create("Ship queue");
  layout_left = Box::Create(Box::Orientation::VERTICAL, 5);
  layout_left_q = Box::Create(Box::Orientation::VERTICAL, 5);
  layout_right = Box::Create(Box::Orientation::VERTICAL, 5);
  layout_right_q = Box::Create(Box::Orientation::VERTICAL, 5);

  wrapper->Pack(layout);
  layout->Pack(frame_left);
  layout->Pack(frame_left_q);
  layout->Pack(frame_right);
  layout->Pack(frame_right_q);
  frame_right->Add(layout_right);
  frame_right_q->Add(layout_right_q);
  frame_left->Add(layout_left);
  frame_left_q->Add(layout_left_q);

  // add buildings
  for (auto v : keywords::development) {
    available_buildings[v] = s->development[v];
  }

  for (auto v : s->choice_data.building_queue) extend_building_queue(v);

  auto bkeys = utility::get_map_keys(available_buildings);
  bkeys.sort();
  for (auto v : bkeys) {
    Button::Ptr b = Button::Create(v + " level " + to_string(available_buildings[v] + 1));
    desktop->bind_ppc(b, [this, v]() {
      extend_building_queue(v);
      update_available_button(v);
    });
    available_buildings_b[v] = b;
    layout_left->Pack(b);
  }

  // add ships
  auto skey = utility::get_map_keys(ship_stats::table());
  skey.sort();
  for (auto v : skey) {
    if (desktop->get_research().can_build_ship(v, s)) {
      Button::Ptr b = Button::Create(v);
      desktop->bind_ppc(b, [this, v]() {
        extend_ship_queue(v);
      });
      layout_right->Pack(b);
    }
  }

  for (auto v : s->choice_data.ship_queue) extend_ship_queue(v);

  // add response buttons
  Button::Ptr b_ok = Button::Create("OK");
  Button::Ptr b_cancel = Button::Create("Cancel");

  desktop->bind_ppc(b_ok, [this, s]() {
    auto process = [](hm_t<int, pair<string, sfg::Button::Ptr> > data) {
      list<pair<int, string> > buf;
      for (auto x : data) buf.push_back(make_pair(x.first, x.second.first));
      buf.sort([](pair<int, string> a, pair<int, string> b) { return a.first < b.first; });

      list<string> result;
      for (auto x : buf) result.push_back(x.second);
      return result;
    };

    s->choice_data.building_queue = process(building_queue);
    s->choice_data.ship_queue = process(ship_queue);

    desktop->clear_qw();
  });

  desktop->bind_ppc(b_cancel, []() {
    desktop->clear_qw();
  });

  Box::Ptr b_layout = Box::Create(Box::Orientation::HORIZONTAL, 5);
  b_layout->Pack(b_ok);
  b_layout->Pack(b_cancel);
  wrapper->Pack(b_layout);
}

void solar_gui::update_available_button(string v) {
  available_buildings_b[v]->SetLabel(v + " level " + to_string(available_buildings[v] + 1));
}

void solar_gui::extend_building_queue(string v) {
  static int bid = 0;
  if (building_queue.size() >= 10) return;

  available_buildings[v]++;
  string name = v + " level " + to_string(available_buildings[v]);
  if (building_queue.empty() && v == sol->choice_data.devstring()) {
    int percent = 100 * sol->build_progress / sol->devtime(v);
    name += ": " + to_string(percent) + "%";
  }

  Button::Ptr b = Button::Create(name);
  int id = bid++;
  building_queue[id] = make_pair(v, b);
  desktop->bind_ppc(b, [this, v, id, name]() {
    cout << "building_queue: remove called on " << name << endl;
    layout_left_q->Remove(building_queue[id].second);
    building_queue.erase(id);
    available_buildings[v]--;
    update_available_button(v);
  });
  layout_left_q->Pack(b);
}

void solar_gui::extend_ship_queue(string v) {
  static int bid = 0;
  if (ship_queue.size() >= 10) return;

  string name = v;
  if (ship_queue.empty() && sol->ship_progress > 0) {
    if (sol->choice_data.do_produce() && sol->choice_data.ship_queue.front() == v) {
      float build_time = ship::table().at(v).build_time;
      int percent = 100 * sol->ship_progress / build_time;
      name += ": " + to_string(percent) + "%";
    } else {
      // we just changed the order (if there was one)
      sol->ship_progress = 0;
    }
  }

  Button::Ptr b = Button::Create(name);
  int id = bid++;
  ship_queue[id] = make_pair(v, b);
  desktop->bind_ppc(b, [this, v, id, name]() {
    cout << "ship_queue: remove called on " << name << endl;
    layout_right_q->Remove(ship_queue[id].second);
    ship_queue.erase(id);
  });
  layout_right_q->Pack(b);
}
