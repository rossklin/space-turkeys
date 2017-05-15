#include "solar_gui.h"

using namespace std;
using namespace st3;
using namespace sfg;
using namespace interface;

const std::string solar_gui::sfg_id = "solar gui";

solar_gui::Ptr solar_gui::Create(solar::ptr s){
  auto buf = Ptr(new solar_gui(s));

  return buf;
}

// main window for solar choice
solar_gui::solar_gui(solar::ptr s) : Window(Window::Style::BACKGROUND), sol(s){
  if (desktop -> response.solar_choices.count(sol -> id)){
    response = desktop -> response.solar_choices[sol -> id];
  }else{
    response = s -> choice_data;
  }

  // main layout
  layout = Box::Create(Box::Orientation::VERTICAL, 10);

  layout -> Pack(Separator::Create(Separator::Orientation::HORIZONTAL));
  tooltip = Label::Create("Customize solar choice for solar " + sol -> id);
  layout -> Pack(tooltip);
  layout -> Pack(Separator::Create(Separator::Orientation::HORIZONTAL));

  auto meta_layout = Box::Create(Box::Orientation::HORIZONTAL, 10);
  meta_layout -> Pack(choice_layout = Box::Create(Box::Orientation::VERTICAL));
  meta_layout -> Pack(sub_layout = Box::Create(Box::Orientation::VERTICAL));
  meta_layout -> Pack(info_layout = Box::Create(Box::Orientation::VERTICAL));
    
  // choice template buttons
  auto template_map = desktop -> get_research().solar_template_table(sol);
  auto template_layout = Box::Create(Box::Orientation::HORIZONTAL);

  for (auto &x : template_map){
    auto b = Button::Create(x.first);
    b -> GetSignal(Widget::OnLeftClick).Connect([this, x] () {
	response = x.second;
	build_choice();
	build_info();
	new_sub("(chose sub edit)");
      });
    b -> GetSignal(Widget::OnMouseEnter).Connect([this,x] () {
	tooltip -> SetText("Use template " + x.first);
      });
    template_layout -> Pack(b);
  }

  layout -> Pack(template_layout);
  layout -> Pack(meta_layout);

  build_choice();
  new_sub("(chose sub edit)");
  build_info();

  auto b_accept = Button::Create("ACCEPT");

  b_accept -> GetSignal(Widget::OnLeftClick).Connect([=] () {
      desktop -> response.solar_choices[sol -> id] = response;
      desktop -> clear_qw();
      cout << "Added solar choice for " << sol -> id << endl;
    });
  
  auto b_cancel = Button::Create("CANCEL");

  b_cancel -> GetSignal(Widget::OnLeftClick).Connect([] () {
      desktop -> clear_qw();
    });

  auto l_response = Box::Create(Box::Orientation::HORIZONTAL);
  l_response -> Pack(b_accept);
  l_response -> Pack(b_cancel);

  layout -> Pack(l_response);

  Add(layout);
  SetAllocation(main_interface::qw_allocation);
  SetId(sfg_id);
}

// build a labeled button to modify priority data
Button::Ptr solar_gui::priority_button(string label, float &data, function<bool()> inc_val, Label::Ptr tip){

  auto label_builder = [] (string label, float data){
    return label + ": " + to_string((int)round(data));
  };

  auto b = Button::Create(label_builder(label, data));
  auto p = b.get();

  b -> GetSignal(Widget::OnLeftClick).Connect([this, &data, p, label, inc_val, label_builder](){
      if (inc_val()) data = floor(data + 1);
      p -> SetLabel(label_builder(label, data));
      build_info();
    });
    
  b -> GetSignal(Widget::OnRightClick).Connect([this, &data, p, label, label_builder](){
      if (data > 0) data = fmax(data - 1, 0);
      p -> SetLabel(label_builder(label, data));
      build_info();
    });

  if (tip){
    b -> GetSignal(Widget::OnMouseEnter).Connect([tip, label](){
	tip -> SetText("Modify priority for " + label);
      });
  }

  return b;
};

void solar_gui::build_choice(){
  // sector allocation buttons
  auto layout = Box::Create(Box::Orientation::VERTICAL);
  hm_t<string, function<void()> > subq;
  subq[keywords::key_mining] = [this] () {build_mining();};
  subq[keywords::key_military] = [this] () {build_military();};

  for (auto v : keywords::sector){
    auto b = priority_button(v, response.allocation[v], [this](){return response.allocation.count() < choice::max_allocation;}, tooltip);

    auto l = Box::Create(Box::Orientation::HORIZONTAL);
    l -> Pack(b);

    if (subq.count(v)){
      // sectors with sub interfaces
      auto sub = Button::Create(">");
      sub -> GetSignal(Widget::OnLeftClick).Connect([v,subq] () {subq.at(v)();});
      sub -> GetSignal(Widget::OnMouseEnter).Connect([this,v] () {tooltip -> SetText("Edit sub choices for " + v);});
      l -> Pack(sub);
    }
    
    layout -> Pack(l);
  }

  auto frame = Frame::Create("Sector priorities");
  frame -> Add(layout);

  choice_layout -> RemoveAll();
  choice_layout -> Pack(frame);
}

void solar_gui::build_info(){
  auto res = Box::Create(Box::Orientation::VERTICAL);
  choice::c_solar c = response;
  c.normalize();
  
  auto patch_label = [this] (Label::Ptr a, string v){
    a -> SetAlignment(sf::Vector2f(0, 0));
    a -> GetSignal(Widget::OnMouseEnter).Connect([this, v] () {
	tooltip -> SetText("Status for " + v + " (rate of change in parenthesis)");
      });
  };    

  auto label_build = [this, patch_label] (string v, float absolute, float delta) -> Label::Ptr{
    auto a = Label::Create(v + ": " + utility::format_float(absolute) + " [" + utility::format_float(delta) + "]");
    patch_label(a, v);
    return a;
  };

  auto resource_label_build = [this, patch_label] (string v, float absolute, float delta, float avail) -> Label::Ptr{
    auto a = Label::Create(v + ": " + utility::format_float(absolute) + " [" + utility::format_float(delta) + "] - " + utility::format_float(avail));
    patch_label(a, v);
    return a;
  };

  auto frame = [] (string title, sfg::Widget::Ptr content) {
    auto buf = Box::Create(Box::Orientation::VERTICAL);
    buf -> Pack(Separator::Create(Separator::Orientation::HORIZONTAL));
    buf -> Pack(Label::Create(title));
    buf -> Pack(Separator::Create(Separator::Orientation::HORIZONTAL));
    buf -> Pack(content);
    return buf;
  };

  auto buf = Box::Create(Box::Orientation::VERTICAL);
  buf -> Pack(label_build("Population", sol -> population, sol -> population_increment()));
  buf -> Pack(label_build("Happiness", sol -> happiness, sol -> happiness_increment(c)));
  buf -> Pack(label_build("Ecology", sol -> ecology, sol -> ecology_increment()));
  buf -> Pack(label_build("Water", sol -> water_status(), 0));
  buf -> Pack(label_build("Space", sol -> space_status(), 0));
  buf -> Pack(label_build("Research rate", sol -> research_increment(c), 0));
  buf -> Pack(label_build("Development points", sol -> development_points, sol -> development_increment(c)));

  res -> Pack(frame("Stats", buf));

  buf = Box::Create(Box::Orientation::VERTICAL);
  for (auto &f : sol -> development) {
    buf -> Pack(label_build(f.first, f.second.level, sol -> development_increment(c)));
  }

  res -> Pack(frame("Sectors", buf));

  buf = Box::Create(Box::Orientation::VERTICAL);
  for (auto v : keywords::resource) {
    buf -> Pack(resource_label_build(v, sol -> resource_storage[v], sol -> resource_increment(v, c), sol -> available_resource[v]));
  }

  res -> Pack(frame("Resources", buf));

  buf = Box::Create(Box::Orientation::VERTICAL);
  buf -> Pack(label_build("Ships", sol -> ships.size(), 0));
  buf -> Pack(label_build("Shield power", sol -> compute_shield_power(), 0));

  res -> Pack(frame("Military", buf));
  
  info_layout -> RemoveAll();
  info_layout -> Pack(frame("Solar " + sol -> id + " info", res), false, false);
}

Box::Ptr solar_gui::new_sub(string v){
  auto buf = Box::Create(Box::Orientation::VERTICAL);
  auto frame = Frame::Create(v);
  frame -> Add(buf);
  sub_layout -> RemoveAll();
  sub_layout -> Pack(frame);
  return buf;
}
 
// sub window for military choice
void solar_gui::build_military(){
  auto &c = response.military;
  auto buf = new_sub("Military build priorities");
  list<string> req;
  hm_t<string, list<string> > unqualified;
  
  // add buttons for expandable sectors
  for (auto v : ship::all_classes()) {
    if (desktop -> get_research().can_build_ship(v, sol, &req)) {
      buf -> Pack(priority_button(v, c[v], [&c](){return c.count() < choice::max_allocation;}, tooltip));
    }else{
      unqualified[v] = req;
    }
  }

  for (auto &x : unqualified) {
    buf -> Pack(Label::Create(x.first));
    auto r = Label::Create(boost::algorithm::join(x.second, ", "));
    string id = "req#" + x.first;
    r -> SetId(id);
    buf -> Pack(r);
    desktop -> SetProperty(id, "Color", sf::Color::Red);
    buf -> Pack(Separator::Create(Separator::Orientation::HORIZONTAL));
  }
};

// sub window for mining choice
void solar_gui::build_mining(){
  auto &c = response.mining;
  auto buf = new_sub("Mining resource priorities");

  // add buttons for expandable sectors
  for (auto v : keywords::resource){
    auto b = priority_button(v, c[v], [&c](){return c.count() < choice::max_allocation;}, tooltip);
    buf -> Pack(b);
  }
};
