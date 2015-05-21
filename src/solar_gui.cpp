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

const string tgui_root("/usr/local/share/tgui-0.6/");

// names of solar gui templates
vector<string> solar_gui::template_name({"population","industry","ship"});

// constructor: assign Gui parent ref to control components
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
  slider -> setPosition(pos.x + dims.x/4 + 0.1 * dims.x / 2, pos.y);
  slider -> setSize(0.8 * dims.x / 2, dims.y / 2);
  slider -> setMaximum(cap);
  slider -> setValue(value);
  slider -> setCallbackId(id);
  slider -> bindCallbackEx(&solar_gui::control_group::update_value, c, Slider::ValueChanged); 

  feedback -> load(conf);
  feedback -> setPosition(pos.x + 3 * dims.x / 4, pos.y);
  feedback -> setSize(dims.x / 4, dims.y);
  feedback -> setText(to_string((int)value) + "(" + to_string((int)cap) + "), inc: " + to_string(deriv));
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
  float local_cap = fmin(total, cap);
  for (auto x : proportions) sum += x;
  if (sum > 1) utility::normalize_vector(proportions);

  label -> load(conf);
  label -> setPosition(pos.x, pos.y);
  label -> setSize(label_dims.x, label_dims.y);
  label -> setText(title + ": " + to_string(g -> s.industry[id]));
  label -> setTextStyle(sf::Text::Bold);
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
			 local_cap * proportions[i],
			 cap,
			 incs(i, local_cap * proportions[i]) * sg -> round_time);
    label_offset += label_dims.y;
  }
}

void solar_gui::control_group::load_controls(float tot, vector<float> p){
  float sum = 0;
  float local_cap = fmin(tot, cap);
  for (auto x : p) sum += x;
  if (sum > 1) utility::normalize_vector(p);

  cout << "control_group::load_controls: sector " << id << " setting total " << tot << " and props " << p << endl;

  for (int i = 0; i < p.size(); i++){
    controls[i] -> slider -> setValue(fmin(cap, tot) * p[i]);
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

  controls[c.id] -> update(incs(c.id, s -> getValue()) * sg -> round_time);
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
solar_gui::solar_gui(window_t &w, solar::solar sol, solar::choice_t cc, research res, float rt) : 
  Gui(w), 
  window(w), 
  s(sol), 
  c(cc), 
  r(res), 
  header_population(*this),
  tsel(*this),
  round_time(rt){
  
  setGlobalFont(graphics::default_font);
  done = false;
  c.normalize();

  point margin(10, 10);
  float bottom_panel_height = 40;
  point dimension = point(w.getDefaultView().getSize().x, w.getDefaultView().getSize().y) - utility::scale_point(margin, 2) - point(0, bottom_panel_height);
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
  controls.resize(solar::work_num, 0);
  vector<vector<float> > values(solar::work_num);

  values[solar::work_ship] = s.fleet_growth;
  values[solar::work_research] = r.field;
  values[solar::work_resource] = s.resource_storage;
  for (int i = 0; i < solar::work_num; i++){
    if (controls[i]) delete controls[i];
    controls[i] = new solar_gui::control_group
      (this,
       i,
       black,
       st3::solar::work_names[i],
       point(margin.x, margin.y + label_offset),
       point(dimension.x, label_size.y),
       st3::solar::sub_names[i],
       values[i],
       c.subsector[i],
       s.population_number * c.workers * c.sector[i],
       bind(&solar::solar::sub_increment, &s, r, i, placeholders::_1, placeholders::_2),
       s.industry[i]
       );

    label_offset += (st3::solar::sub_names[i].size() + 1) * label_size.y;
  }

  update_popsum();

  // template selector
  float template_y = margin.y + dimension.y - 60;
  Label::Ptr tlabel(*this);
  tlabel -> load(black);
  tlabel -> setSize(80, 30);
  tlabel -> setPosition(margin.x, template_y);
  tlabel -> setText("Load template:");
  tlabel -> setTextSize(12);

  tsel -> load(black);
  tsel -> setSize(0.8 * dimension.x / 2, 20);
  tsel -> setPosition(margin.x + 1.2 * dimension.x / 4, template_y);
  for (auto x : template_name) tsel -> addItem(x);
  tsel -> bindCallback(bind(&solar_gui::callback_template, this), ComboBox::ItemSelected);
  
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

void solar_gui::callback_template(){
  load_template(tsel -> getSelectedItem());
}

void solar_gui::load_controls(solar::choice_t ch){
  c = ch;
  c.normalize();

  for (int i = 0; i < controls.size(); i++){
    controls[i] -> load_controls(s.population_number * c.workers * c.sector[i], c.subsector[i]);
  }

  update_popsum();
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
  vector<string> happy_string({":C", ":(", ":|", ":)", ":D"});
  header_population -> setText(happy_string[happy_string.size() * fmin(s.population_happy, 0.99)] + " Population: " + to_string(s.population_number) + "(farmers: " + to_string(s.population_number - sum) + ", inc: " + to_string(s.pop_increment(r, s.population_number - sum) * round_time) + "), def: " + to_string(s.defense_current) + " [" + to_string(s.defense_capacity) + "]");
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
	for (int j = 0; j < st3::solar::sub_names[i].size(); j++){
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
      cout << "solar gui window close not handled!" << endl;
      exit(-1);
    }

    sf::Event event;
    while (window.pollEvent(event)){
      if (event.type == sf::Event::Closed) window.close();
      handleEvent(event);
      if (event.type == sf::Event::KeyPressed){
	switch (event.key.code){
	case sf::Keyboard::Return:
	  callback_done(true);
	  break;
	case sf::Keyboard::Escape:
	  callback_done(false);
	  break;
	}
      }
    }

    window.clear();
    draw();
    window.display();
    sf::sleep(sf::milliseconds(100));
  }

  c = evaluate();

  return accept;
}

float solar_gui::compute_workers_nostarve(float priority){
  float n = s.population_number;
  // dp/dt = a f n + b n
  float dmax = s.pop_increment(r, n);
  float dmin = s.pop_increment(r, 0);
  float dsel = fmin(dmax, fmax(0, dmin + priority * (dmax - dmin)));
  float b = dmin / n;
  float a = (dmax - b * n) / pow(n, 2);
  float f = (dsel - b * n) / (a * n);
  return (n - f) / n;
}

float solar_gui::compute_workers(float priority){
  float n = s.population_number;
  // dp/dt = a f p + b p
  float dmax = s.pop_increment(r, n);
  float dmin = s.pop_increment(r, 0);
  float dsel = dmin + priority * (dmax - dmin);
  float b = dmin / n;
  float a = (dmax - b * n) / pow(n, 2);
  float f = (dsel - b * n) / (a * n);
  return (n - f) / n;
}

void solar_gui::load_template(string s){
  solar::choice_t c;
  for (auto &x : c.sector) x = 0;
  for (auto &x : c.subsector) for (auto &y : x) y = 0;

  if (!s.compare("population")){
    c.workers = compute_workers_nostarve(0.8);
    c.sector[solar::work_research] = 1;
    c.subsector[solar::work_research][research::r_population] = 1;
  }else if (!s.compare("industry")){
    c.workers = compute_workers_nostarve(0.5);
    c.sector[solar::work_expansion] = 1;
    c.sector[solar::work_research] = 0.5;
    c.sector[solar::work_resource] = 0.5;
    c.subsector[solar::work_expansion][solar::work_expansion] = 1;
    c.subsector[solar::work_expansion][solar::work_ship] = 0.2;
    c.subsector[solar::work_expansion][solar::work_research] = 0.4;
    c.subsector[solar::work_expansion][solar::work_resource] = 0.2;
    c.subsector[solar::work_research][research::r_industry] = 1;
    for (auto &x : c.subsector[solar::work_resource]) x = 1;
  }else if (!s.compare("ship")){
    c.workers = compute_workers_nostarve(0.3);
    c.sector[solar::work_expansion] = 1;
    c.sector[solar::work_ship] = 2;
    c.sector[solar::work_resource] = 1;
    c.subsector[solar::work_expansion][solar::work_ship] = 1;
    for (auto &x : c.subsector[solar::work_ship]) x = 1;
    for (auto &x : c.subsector[solar::work_resource]) x = 1;
  }else{
    cout << "load_template: unknown: " << s << endl;
    exit(-1);
  }

  load_controls(c);
}
