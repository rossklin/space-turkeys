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
solar_gui::solar_gui(sf::RenderWindow &w, solar::solar &sol) : Gui(w), window(w), tabs(*this), s(sol){
  setGlobalFont(graphics::default_font);
  done = false;

  point margin(10, 10);
  float bottom_panel_height = 40;
  point dimension = point(w.getSize().x, w.getSize().y) - utility::scale_point(margin, 2) - point(0, bottom_panel_height);
  point label_size(dimension.x / 2, 20);
  float label_offset = 0;
  string black = tgui_root + "widgets/Black.conf";
  list<pair<string, string> > label_text;

  // setup panels
  topics = {
    "Overview", 
    "Population", 
    "Industry", 
    "Shipyard",
    "Research",
    "Resource"
  };

  tabs -> load(black);
  tabs -> setPosition(margin.x, margin.y);
  for (auto x : topics) tabs -> add(x);
  tabs -> select(topics[0]);
  tabs -> bindCallbackEx(&solar_gui::callback_panel, this, Tab::TabChanged);

  for (auto x : topics){
    panels[x] = Panel::Ptr(*this);
    panels[x] -> setSize(dimension.x, dimension.y - tabs -> getTabHeight());
    panels[x] -> setPosition(margin.x, margin.y + tabs -> getTabHeight());
    panels[x] -> setBackgroundColor(sf::Color::Transparent);
    panels[x] -> hide();
  }

  // setup overview
  label_text.push_back(make_pair("Population", to_string(s.population_number) + "(happy: " + to_string(s.population_happy * s.population_number) + ")"));
  label_text.push_back(make_pair("Defense", to_string(s.defense_current) + "(cap: " + to_string(s.defense_capacity) + ")"));

  label_text.push_back(make_pair("Industries", ""));
  label_text.push_back(make_pair(" - Infrastructure", to_string(s.dev.industry[solar::i_infrastructure])));
  label_text.push_back(make_pair(" - Agriculture", to_string(s.dev.industry[solar::i_agriculture])));
  label_text.push_back(make_pair(" - Shipyard", to_string(s.dev.industry[solar::i_ship])));
  label_text.push_back(make_pair(" - Resource mining", to_string(s.dev.industry[solar::i_resource])));
  label_text.push_back(make_pair(" - Research facilities", to_string(s.dev.industry[solar::i_research])));

  label_text.push_back(make_pair("Resources", ""));
  label_text.push_back(make_pair(" - Metal", to_string(s.dev.resource[solar::o_metal])));
  label_text.push_back(make_pair(" - Gas", to_string(s.dev.resource[solar::o_gas])));

  label_text.push_back(make_pair("Shipyard", ""));
  label_text.push_back(make_pair(" - Scout", to_string(s.dev.fleet_growth[solar::s_scout])));
  label_text.push_back(make_pair(" - Fighter", to_string(s.dev.fleet_growth[solar::s_fighter])));
  label_text.push_back(make_pair(" - Bomber", to_string(s.dev.fleet_growth[solar::s_bomber])));

  for (auto x : label_text){
    Label::Ptr ll, lr;

    ll -> load(black);
    ll -> setPosition(margin.x, margin.y + label_offset);
    ll -> setSize(label_size.x, label_size.y);
    ll -> setText(x.first + ":");
    ll -> setTextSize(12);
    panels["Overview"] -> add(ll);

    lr -> load(black);
    lr -> setPosition(margin.x + dimension.x / 2, margin.y + label_offset);
    lr -> setSize(label_size.x, label_size.y);
    lr -> setText(x.second);
    lr -> setTextSize(12);
    panels["Overview"] -> add(lr);
    label_offset += label_size.y;
  }

  // headers
  label_size.x = dimension.x;
  label_size.y = 40;
  label_offset = 60;

  header_population -> load(black);
  header_population -> setPosition(margin.x, 0);
  header_population -> setSize(label_size.x, label_size.y);
  panels["Population"] -> add(header_population);
  header_industry -> load(black);
  header_industry -> setPosition(margin.x, 0);
  header_industry -> setSize(label_size.x, label_size.y);
  panels["Industry"] -> add(header_industry);
  header_research -> load(black);
  header_research -> setPosition(margin.x, 0);
  header_research -> setSize(label_size.x, label_size.y);
  panels["Research"] -> add(header_research);
  header_resource -> load(black);
  header_resource -> setPosition(margin.x, 0);
  header_resource -> setSize(label_size.x, label_size.y);
  panels["Resource"] -> add(header_resource);
  header_ship -> load(black);
  header_ship -> setPosition(margin.x, 0);
  header_ship -> setSize(label_size.x, label_size.y);
  panels["Shipyard"] -> add(header_ship);

  // setup population
  label_size.x = dimension.x / 4;

  population_slider.resize(solar::p_num);
  for (int i = 0; i < solar::p_num; i++){
    Label::Ptr lpop;
    lpop -> load(black);
    lpop -> setPosition(margin.x, margin.y + label_offset + i * label_size.y);
    lpop -> setSize(label_size.x, label_size.y);
    lpop -> setText(solar::development::population_names[i] + ":");
    panels["Population"] -> add(lpop);

    population_slider[i] -> load(black);
    population_slider[i] -> setPosition(margin.x + label_size.x, margin.y + label_offset + i * label_size.y);
    population_slider[i] -> setVerticalScroll(false);
    population_slider[i] -> setSize(dimension.x - label_size.x - margin.x, label_size.y / 3);
    population_slider[i] -> setMaximum(slider_count);
    population_slider[i] -> bindCallback(&solar_gui::update_info, this, Slider::ValueChanged);
    panels["Population"] -> add(population_slider[i]);
  }

  // setup industry
  industry_slider.resize(solar::i_num);
  for (int i = 0; i < solar::i_num; i++){
    Label::Ptr lind;
    lind -> load(black);
    lind -> setPosition(margin.x, margin.y + label_offset + i * label_size.y);
    lind -> setSize(label_size.x, label_size.y);
    lind -> setText(solar::development::industry_names[i] + ":");
    panels["Industry"] -> add(lind);

    industry_slider[i] -> load(black);
    industry_slider[i] -> setPosition(margin.x + label_size.x, margin.y + label_offset + i * label_size.y);
    industry_slider[i] -> setVerticalScroll(false);
    industry_slider[i] -> setSize(dimension.x - label_size.x - margin.x, label_size.y / 3);
    industry_slider[i] -> setMaximum(slider_count);
    industry_slider[i] -> bindCallback(&solar_gui::update_info, this, Slider::ValueChanged);
    panels["Industry"] -> add(industry_slider[i]);
  }  

  // setup research
  research_slider.resize(research::r_num);
  for (int i = 0; i < research::r_num; i++){
    Label::Ptr lind;
    lind -> load(black);
    lind -> setPosition(margin.x, margin.y + label_offset + i * label_size.y);
    lind -> setSize(label_size.x, label_size.y);
    lind -> setText(solar::development::research_names[i] + ":");
    panels["Research"] -> add(lind);

    research_slider[i] -> load(black);
    research_slider[i] -> setPosition(margin.x + label_size.x, margin.y + label_offset + i * label_size.y);
    research_slider[i] -> setVerticalScroll(false);
    research_slider[i] -> setSize(dimension.x - label_size.x - margin.x, label_size.y / 3);
    research_slider[i] -> setMaximum(slider_count);
    research_slider[i] -> bindCallback(&solar_gui::update_info, this, Slider::ValueChanged);
    panels["Research"] -> add(research_slider[i]);
  }  

  // setup shipyard
  ship_slider.resize(solar::s_num);
  for (int i = 0; i < solar::s_num; i++){
    Label::Ptr lind;
    lind -> load(black);
    lind -> setPosition(margin.x, margin.y + label_offset + i * label_size.y);
    lind -> setSize(label_size.x, label_size.y);
    lind -> setText(solar::development::ship_names[i] + ":");
    panels["Shipyard"] -> add(lind);

    ship_slider[i] -> load(black);
    ship_slider[i] -> setPosition(margin.x + label_size.x, margin.y + label_offset + i * label_size.y);
    ship_slider[i] -> setVerticalScroll(false);
    ship_slider[i] -> setSize(dimension.x - label_size.x - margin.x, label_size.y / 3);
    ship_slider[i] -> setMaximum(slider_count);
    ship_slider[i] -> bindCallback(&solar_gui::update_info, this, Slider::ValueChanged);
    panels["Shipyard"] -> add(ship_slider[i]);
  }  

  // setup resource
  resource_slider.resize(solar::o_num);
  for (int i = 0; i < solar::o_num; i++){
    Label::Ptr lind;
    lind -> load(black);
    lind -> setPosition(margin.x, margin.y + label_offset + i * label_size.y);
    lind -> setSize(label_size.x, label_size.y);
    lind -> setText(solar::development::resource_names[i] + ":");
    panels["Resource"] -> add(lind);

    resource_slider[i] -> load(black);
    resource_slider[i] -> setPosition(margin.x + label_size.x, margin.y + label_offset + i * label_size.y);
    resource_slider[i] -> setVerticalScroll(false);
    resource_slider[i] -> setSize(dimension.x - label_size.x - margin.x, label_size.y / 3);
    resource_slider[i] -> setMaximum(slider_count);
    resource_slider[i] -> bindCallback(&solar_gui::update_info, this, Slider::ValueChanged);
    panels["Resource"] -> add(resource_slider[i]);
  }

  panels["Overview"] -> show();

  Button::Ptr b(*this);
  b -> load(black);
  b -> setSize(200, 40);
  b -> setPosition(margin.x + dimension.x / 2, margin.y + dimension.y);
  b -> setText("ACCEPT");
  b -> bindCallback(bind(&solar_gui::callback_done, this, true), Button::LeftMouseClicked);

  Button::Ptr bc(*this);
  bc -> load(black);
  bc -> setSize(200, 40);
  bc -> setPosition(margin.x + dimension.x / 2 - bc -> getSize().x, margin.y + dimension.y);
  bc -> setText("CANCEL");
  bc -> bindCallback(bind(&solar_gui::callback_done, this, false), Button::LeftMouseClicked);
}

float solar_gui::slider_sum(vector<Slider::Ptr> &v){
  float sum = 0;
  for (auto &x : v) sum += x -> getValue();
  return sum;
}

void solar_gui::update_info(){
  header_population -> setText("Total population: " + to_string(s.population_number));

  header_industry -> setText("Available workers: " + to_string(population_slider[solar::p_industry] -> getValue() / slider_sum(population_slider) * s.population_number));

  header_research -> setText("Available researchers: " + to_string(population_slider[solar::p_research] -> getValue() / slider_sum(population_slider) * s.population_number));

  header_resource -> setText("Available miners: " + to_string(population_slider[solar::p_resource] -> getValue() / slider_sum(population_slider) * s.population_number));

  header_ship -> setText("Available ship engineers: " + to_string(population_slider[solar::p_industry] -> getValue() * industry_slider[solar::i_ship] -> getValue() / (slider_sum(population_slider) * slider_sum(industry_slider)) * s.population_number));
}

void solar_gui::callback_done(bool a){
  accept = a;
  done = true;
}

void solar_gui::callback_panel(const Callback &c){
  Tab *tabs = (Tab*)c.widget;
  for (auto &x : panels) x.second -> hide();
  panels[tabs -> getSelected()] -> show();
}

void solar_gui::setup_controls(solar::choice_t c){
  for (int i = 0; i < solar::p_num; i++){
    population_slider[i] -> setValue(slider_count * c.population[i]);
  }

  for (int i = 0; i < solar::i_num; i++){
    industry_slider[i] -> setValue(slider_count * c.dev.industry[i]);
  }

  for (int i = 0; i < research::r_num; i++){
    research_slider[i] -> setValue(slider_count * c.dev.new_research[i]);
  }

  for (int i = 0; i < solar::o_num; i++){
    resource_slider[i] -> setValue(slider_count * c.dev.resource[i]);
  }

  for (int i = 0; i < solar::s_num; i++){
    ship_slider[i] -> setValue(slider_count * c.dev.fleet_growth[i]);
  }

  update_info();
}

solar::choice_t solar_gui::evaluate(){
  solar::choice_t c;
  for (int i = 0; i < solar::p_num; i++){
    c.population[i] = population_slider[i] -> getValue() / (float)slider_count;
  }

  for (int i = 0; i < research::r_num; i++){
    c.dev.new_research[i] = research_slider[i] -> getValue() / (float)slider_count;
  }

  for (int i = 0; i < solar::i_num; i++){
    c.dev.industry[i] = industry_slider[i] -> getValue() / (float)slider_count;
  }

  for (int i = 0; i < solar::o_num; i++){
    c.dev.resource[i] = resource_slider[i] -> getValue() / (float)slider_count;
  }

  for (int i = 0; i < solar::s_num; i++){
    c.dev.fleet_growth[i] = ship_slider[i] -> getValue() / (float)slider_count;
  }

  return c;
}

bool solar_gui::run(solar::choice_t &c){
  setup_controls(c);

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

  c = evaluate();

  return accept;
}
