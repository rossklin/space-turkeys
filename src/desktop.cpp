#include "desktop.h"
#include "client_game.h"
#include "solar_gui.h"
#include "research_gui.h"

using namespace std;
using namespace st3;
using namespace sfg;
using namespace interface;

sf::Vector2u main_interface::desktop_dims;
sf::FloatRect main_interface::qw_allocation;
int main_interface::top_height;
int main_interface::bottom_start;

main_interface *interface::desktop = 0;

main_interface::main_interface(sf::Vector2u d, client::game *gx) : g(gx) {
  done = false;
  accept = false;
  desktop_dims = d;
  
  // build geometric data
  top_height = 0.1 * d.y;
  bottom_start = 0.9 * d.y;
  int qw_top = 10;
  int qw_bottom = 0.85 * d.y;
  qw_allocation = sf::FloatRect(10, qw_top, d.x - 20, qw_bottom - qw_top);

  auto get_rect = [] (float a, float b, float c, float d) {
    return sf::FloatRect(a * desktop_dims.x, b * desktop_dims.y, c * desktop_dims.x, d * desktop_dims.y);
  };

  auto pack_in_window = [] (Widget::Ptr w, sf::FloatRect alloc) -> Window::Ptr {
    auto window = Window::Create(Window::Style::BACKGROUND);
    auto box = Box::Create();
    box -> Pack(w);
    window -> Add(box);
    window -> SetAllocation(alloc);
    return window;
  };

  // add proceed button
  auto b_proceed = Button::Create("PROCEED");
  b_proceed -> GetSignal(Widget::OnLeftClick).Connect([this](){
      done = true;
      accept = true;
    });

  auto w_proceed = pack_in_window(b_proceed, get_rect(0.3, 0.9, 0.4, 0.1));
  Add(w_proceed);

  // hover info label
  Box::Ptr right_panel = Box::Create(Box::Orientation::VERTICAL, 10);
  Button::Ptr b_research = Button::Create("Research");
  bind_ppc(b_research, [this] () {reset_qw(research_gui::Create());});
  hover_label = Label::Create("Empty space");
  log_panel = Box::Create(Box::Orientation::VERTICAL);
  
  right_panel -> Pack(b_research, true, false);
  right_panel -> Pack(graphics::wrap_in_scroll2(log_panel, 0.17 * desktop_dims.x, 0.4 * desktop_dims.y));
  right_panel -> Pack(graphics::wrap_in_scroll2(hover_label, 0.17 * desktop_dims.x, 0.4 * desktop_dims.y));

  auto info_rect = get_rect(0.79, 0.01, 0.2, 0.98);
  right_panel -> SetAllocation(info_rect);
  auto w_info = pack_in_window(right_panel, info_rect);
  Add(w_info);

  // set display properties
  SetProperty("Window#" + string(solar_gui::sfg_id), "BackgroundColor", sf::Color(20, 30, 120, 100));
  SetProperty("Button", "BackgroundColor", sf::Color(20, 120, 80, 140));
  SetProperty("Button", "BorderColor", sf::Color(80, 180, 120, 200));
  SetProperty("Widget", "Color", sf::Color(200, 170, 120));
}

void main_interface::push_log(list<string> log) {
  static list<string> queue;
  int max_size = 40;

  for (auto v : log) queue.push_front(v);
  while (queue.size() > max_size) queue.pop_back();

  log_panel -> RemoveAll();
  for (auto v : queue) log_panel -> Pack(Label::Create(v), true, false);
}

void main_interface::bind_ppc(Widget::Ptr w, function<void(void)> f) {
  w -> GetSignal(Widget::OnLeftClick).Connect([this, f] () {
      post_process.push_back(f);
    });
}

// let sfg::Desktop handle event, then run post processing callbacks
// added by children's event handlers
void main_interface::HandleEvent(const sf::Event& event) {
  Desktop::HandleEvent(event);
  for (auto f : post_process) f();
  post_process.clear();
}

research::data main_interface::get_research(){
  return g -> players[g -> self_id].research_level;
}

void main_interface::reset_qw(Widget::Ptr w){
  clear_qw();
  if (w == 0) return;
  
  query_window = w;
  Add(query_window);
}

void main_interface::clear_qw(){
  if (query_window) Remove(query_window);
  query_window = 0;
}
