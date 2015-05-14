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

solar_gui::control::control(Gui &g){
  label = Label::Ptr(g);
  slider = Slider::Ptr(g);
  feedback = Label::Ptr(g);
}

void solar_gui::control::setup(solar_gui::control_group *c,
			       int id,
			       string conf, 
			       point pos, 
			       point dims, 
			       string name, 
			       float base_value,
			       float value, 
			       float cap, 
			       float deriv){
  label -> load(conf);
  label -> setPosition(pos.x, pos.y);
  label -> setSize(dims.x/4, dims.y);
  label -> setText(name + (base_value >= 0 ? ": " + to_string(base_value) : ""));
  label -> setTextSize(12);

  slider -> load(conf);
  slider -> setVerticalScroll(false);
  slider -> setPosition(pos.x + dims.x/4, pos.y);
  slider -> setSize(dims.x / 2, dims.y);
  slider -> setMaximum(cap);
  slider -> setValue(value);
  slider -> setCallbackId(id);
  slider -> bindCallbackEx(&solar_gui::control_group::update_value, c, Slider::ValueChanged); 

  feedback -> load(conf);
  feedback -> setPosition(pos.x + 3 * dims.x / 4, pos.y);
  feedback -> setSize(dims.x / 4, dims.y);
  feedback -> setText(to_string(value) + "(" + to_string(cap) + "), inc: " + to_string(deriv));
  feedback -> setTextSize(12);
}

void solar_gui::control::update(float deriv){
  feedback -> setText(to_string(slider -> getValue()) + "(" + to_string(slider -> getMaximum()) + "), inc: " + to_string(deriv));
}

solar_gui::control_group::control_group(solar_gui *g, 
					int i,
					string conf,
					string title,
					point pos,
					point label_dims,
					vector<string> names,
					vector<float> values,
					vector<float> proportions,
					float total,
					solar_gui::incfunc fincs,
					float c) : 
  label((Gui&)*g), 
  cap(c), 
  incs(fincs), 
  sg(g), 
  id(i){

  int n = names.size();
  float label_offset = label_dims.y;
  float sum = 0;
  for (auto x : proportions) sum += x;
  if (sum > 1) utility::normalize_vector(proportions);

  label -> load(conf);
  label -> setPosition(pos.x, pos.y);
  label -> setSize(label_dims.x, label_dims.y);
  label -> setText(title + ": " + to_string(g -> s.dev.industry[id]));
  label -> setTextSize(12);

  cout << "control_group: total = " << total << ", props = " << proportions << endl;
  
  controls.resize(n);
  for (int i = 0; i < n; i++){
    controls[i] = new control((Gui&)*g);
    controls[i] -> setup(this,
			 i,
			 conf,
			 point(pos.x, pos.y + label_offset),
			 label_dims,
			 names[i], 
			 values.empty() ? -1 : values[i],
			 fmin(total, cap) * proportions[i],
			 cap,
			 incs(i, cap * proportions[i]));
    label_offset += label_dims.y;
  }
}

void solar_gui::control_group::update_value(Callback const& c){
  Slider *s = (Slider*)c.widget;
  float sum = 0;
  float scap = fmin(cap, sg -> get_control_cap(id));
  for (auto x : controls) sum += x -> slider -> getValue();
  sum -= s -> getValue();

  if (sum > scap){
    cout << "update_value: exclusive sum exceeds cap!" << endl;
    exit(-1);
  }

  if (sum + s -> getValue() > scap){
    s -> setValue(scap - sum);
  }

  controls[c.id] -> update(incs(c.id, s -> getValue()));
  sg -> update_popsum();
}

float solar_gui::control_group::get_sum(){
  float s = 0;
  for (auto x : controls) s += x -> slider -> getValue();
  return s;
}

solar_gui::control_group::~control_group(){
  for (auto x : controls) delete x;
}

// should draw and handle event
solar_gui::solar_gui(sf::RenderWindow &w, solar::solar sol, solar::choice_t &cc, research res) : Gui(w), window(w), s(sol), c(cc), r(res), header_population(*this){
  setGlobalFont(graphics::default_font);
  done = false;

  point margin(10, 10);
  float bottom_panel_height = 40;
  point dimension = point(w.getSize().x, w.getSize().y) - utility::scale_point(margin, 2) - point(0, bottom_panel_height);
  point label_size(dimension.x / 4, 20);
  float label_offset = margin.y;
  string black = tgui_root + "widgets/Black.conf";

  // setup overview
  header_population -> load(black);
  header_population -> setPosition(margin.x, label_offset);
  header_population -> setSize(dimension.x, label_size.y);
  header_population -> setTextSize(12);

  label_offset += label_size.y;

  // setup controls
  controls.resize(solar::work_num);
  for (int i = 0; i < solar::work_num; i++){
    vector<float> values;
    
    switch(i){
    case solar::work_expansion:
      values.clear();
      break;
    case solar::work_ship:
      values = s.dev.fleet_growth;
      break;
    case solar::work_research:
      values = r.field;
      break;
    case solar::work_resource:
      values = s.dev.resource;
      break;
    }

    incfunc f = bind(&solar::solar::sub_increment, &s, r, i, placeholders::_1, placeholders::_2);
    controls[i] = new solar_gui::control_group
      (this,
       i,
       black,
       solar::development::work_names[i],
       point(margin.x, margin.y + label_offset),
       point(dimension.x, label_size.y),
       solar::development::sub_names[i],
       values,
       c.subsector[i],
       s.population_number * c.workers * c.sector[i],
       f,
       s.dev.industry[i]
       );

    label_offset += (solar::development::sub_names[i].size() + 1) * label_size.y;
  }

  update_popsum();

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

solar_gui::~solar_gui(){
  for (auto x : controls) delete x;
}

void solar_gui::callback_done(bool a){
  accept = a;
  done = true;
  cout << "solar_gui: set accept = " << a << endl;
}

float solar_gui::get_control_cap(int id){
  float sum = 0;
  for (auto x : controls) sum += x -> get_sum();
  sum -= controls[id] -> get_sum();
  return s.population_number - sum;
}

void solar_gui::update_popsum(){
  float sum = 0;
  for (auto x : controls) sum += x -> get_sum();
  header_population -> setText("Population: " + to_string(s.population_number) + "(farmers: " + to_string(s.population_number - sum) + ", inc: " + to_string(s.pop_increment(r, s.population_number - sum)) + ")");
}

solar::choice_t solar_gui::evaluate(){
  solar::choice_t c;
  cout << "solar_gui::evaluate" << endl;
  if (s.population_number <= 0) return c;

  float worker_sum = 0;
  for (auto x : controls) worker_sum += x -> get_sum();
  c.workers = worker_sum / s.population_number;

  if (worker_sum > 0){
    for (int i = 0; i < solar::work_num; i++){
      c.sector[i] = controls[i] -> get_sum() / worker_sum;
    }

    for (int i = 0; i < solar::work_num; i++){
      float csum = controls[i] -> get_sum();
      if (csum > 0){
	for (int j = 0; j < solar::development::sub_names[i].size(); j++){
	  c.subsector[i][j] = controls[i] -> controls[j] -> slider -> getValue() / csum;
	}
      }
      cout << "subsector " << i << ": " << c.subsector[i] << endl;
    }

    cout << "sector: " << c.sector << endl;
  }
  
  cout << "solar_gui::evaluate: returning" << endl;
  return c;
}

bool solar_gui::run(){
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
