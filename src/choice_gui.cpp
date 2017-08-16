#include <set>
#include <boost/algorithm/string/join.hpp>

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

choice_gui::choice_gui(string _title, bool _unique, set<string> _options, f_info_t _info, f_result_t _callback) : Bin() {
  title = _title;
  unique = _unique;
  options = _options;
  f_info = _info;
  callback = _callback;
  width = 600;

  // do "css" stuff
  Selector::Ptr sel = Selector::Create( "Button", "", class_selected);
  SetProperty(sel, "BorderWidth", 3);
  SetProperty(sel, "BorderColor", sf::Color::White);
  sel = Selector::Create( "Button", "", class_normal);
  SetProperty(sel, "BorderWidth", 1);
  SetProperty(sel, "BorderColor", sf::Color::Grey);

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
    set_class(v, false);
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
