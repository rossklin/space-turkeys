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

choice_gui::Ptr choice_gui::Create(string title, set<string> options, f_info_t info, f_result_t callback) {
  Ptr buf(new choice_gui(title, options, info, callback));
  buf -> Add(buf -> frame);
  buf -> SetId(sfg_id);
  return buf;
}

choice_gui::choice_gui(string _title, set<string> _options, f_info_t _info, f_result_t _callback) : Bin() {
  title = _title;
  options = _options;
  info = _info;
  callback = _callback;
  width = 600;

  // do "css" stuff
  Selector::Ptr sel = Selector::Create( "Button", "", "choice-selected");
  SetProperty(sel, "BorderWidth", 3);
  SetProperty(sel, "BorderColor", sf::Color::White);
  sel = Selector::Create( "Button", "", "choice-normal");
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

void choice_gui::set_class(string v, bool s) {
  string class_string = "choice-normal";
  if (s) class_string = "choice-selected";
  if (buttons.count(v)) buttons[v] -> SetClass(class_string);
}

void choice_gui::setup() {
  layout -> RemoveAll();

  auto build = [this] (string v) -> Widget::Ptr {
    // add image button
    auto b = Button::Create();
    buttons[v] = b;
    set_class(v, false);
    choice_info info = f_info(v);
    b -> SetImage(Image::Create(graphics::selector_card(v, info.available, info.progress, info.info, info.requirements)));

    // set on_select callback
    if (info.available) {
      desktop -> bind_ppc(b, [this, v] () {
	  string previous = selected;
	  selected = v;
	  set_class(v);
	  set_class(selected);
	});
    }
    
    return b;
  };

  // options go in a scrolled window
  Box::Ptr window_box = Box::Create(Box::Orientation::HORIZONTAL, 5);
  for (auto v : options) window_box -> Pack(build(v));
  layout -> Pack(graphics::wrap_in_scroll(window_box, true, width));

  // buttons for ok/cancel
  Box::Ptr confirm_box = Box::Create(Box::Orientation::HORIZONTAL, 5);
  auto b_cancel = Button::Create("Cancel");
  desktop -> bind_ppc(b_cancel, [this] () {
      callback("", false);
      desktop -> clear_qw();
    });
  
  auto b_accept = Button::Create("Accept");
  desktop -> bind_ppc(b_cancel, [this] () {
      callback(selected, true);
      desktop -> clear_qw();
    });
}
