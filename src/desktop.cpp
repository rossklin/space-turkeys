#include "desktop.h"
#include "client_game.h"
#include "solar_gui.h"

using namespace std;
using namespace st3;
using namespace sfg;
using namespace interface;

sf::Vector2u main_interface::desktop_dims;
sf::FloatRect main_interface::qw_allocation;
int main_interface::top_height;
int main_interface::bottom_start;

main_interface *interface::desktop;

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
    // box -> SetRequisition(hl_window -> GetRequisition());
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
  hover_label = Label::Create("Empty space");
  auto w_info = pack_in_window(hover_label, get_rect(0.71, 0.8, 0.28, 0.19));
  Add(w_info);

  // set display properties
  SetProperty("Window#" + string(solar_gui::sfg_id), "BackgroundColor", sf::Color(20, 30, 120, 100));
  SetProperty("Button", "BackgroundColor", sf::Color(20, 120, 80, 140));
  SetProperty("Button", "BorderColor", sf::Color(80, 180, 120, 200));
  SetProperty("Widget", "Color", sf::Color(200, 170, 120));
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
