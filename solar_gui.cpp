#include <list>
#include <string>

#include <TGUI/TGUI.hpp>

#include "types.h"
#include "solar.h"
#include "solar_gui.h"
#include "graphics.h"
#include "utility.h"

using namespace std;
using namespace tgui;
using namespace st3;

const string tgui_root("external/share/tgui-0.6/");

// should draw and handle event
solar_gui::solar_gui(sf::RenderWindow &w, solar::solar &s) : Gui(w), window(w), tabs(*this){
  setGlobalFont(graphics::default_font);
  done = false;

  point margin(10, 10);
  float bottom_panel_height = 40;
  point dimension = point(w.getSize().x, w.getSize().y) - utility::scale_point(margin, 2) - point(0, bottom_panel_height);
  point label_size(dimension.x, 40);
  float label_offset = 0;
  string black = tgui_root + "widgets/Black.conf";
  list<string> label_text;

  // setup panels
  topics = {"Overview", "Population"};
  tabs -> load(black);
  tabs -> setPosition(margin.x, margin.y);
  for (auto x : topics) tabs -> add(x);
  tabs -> select(topics[0]);
  tabs -> bindCallbackEx(&solar_gui::callback_panel, this, Tab::TabChanged);

  for (auto x : topics){
    panels[x] = Panel::Ptr(*this);
    panels[x] -> setSize(dimension.x, dimension.y - tabs -> getTabHeight());
    panels[x] -> setPosition(margin.x, margin.y + tabs -> getTabHeight());
    panels[x] -> hide();
  }

  // setup overview
  label_text.push_back("Population: " + to_string(s.population_number));
  label_text.push_back("Defense: " + to_string(s.defense_current));

  for (auto x : label_text){
    Label::Ptr l;
    l -> load(black);
    l -> setPosition(margin.x, margin.y + label_offset);
    l -> setSize(label_size.x, label_size.y);
    l -> setText(x);
    panels["Overview"] -> add(l);
    label_offset += label_size.y;
  }

  panels["Overview"] -> show();

  Button::Ptr b(*this);
  b -> load(black);
  b -> setSize(100, 40);
  b -> setPosition(margin.x + dimension.x / 2, margin.y + dimension.y);
  b -> setText("ACCEPT");
  b -> bindCallback(&solar_gui::callback_accept, this, Button::LeftMouseClicked);

  Button::Ptr bc(*this);
  bc -> load(black);
  bc -> setSize(100, 40);
  bc -> setPosition(margin.x + dimension.x / 2 - bc -> getSize().x, margin.y + dimension.y);
  bc -> setText("CANCEL");
  bc -> bindCallback(&solar_gui::callback_cancel, this, Button::LeftMouseClicked);
}

void solar_gui::callback_accept(){
  accept = done = true;
}

void solar_gui::callback_cancel(){
  accept = false;
  done = true;
}

void solar_gui::callback_panel(const Callback &c){
  Tab *tabs = (Tab*)c.widget;
  for (auto &x : panels) x.second -> hide();
  panels[tabs -> getSelected()] -> show();
}

bool solar_gui::run(solar::choice_t &c){
  while (!done){
    if (!window.isOpen()) {
      return false;
    }

    sf::Event event;
    while (window.pollEvent(event)){
      if (event.type == sf::Event::Closed) window.close();
      handleEvent(event);
    }

    window.clear();
    draw();
    window.display();
    sf::sleep(sf::milliseconds(100));
  }

  return accept;
}
