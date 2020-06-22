#include "desktop.h"

#include "choice_gui.h"
#include "client_game.h"

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

  auto get_rect = [](float a, float b, float c, float d) {
    return sf::FloatRect(a * desktop_dims.x, b * desktop_dims.y, c * desktop_dims.x, d * desktop_dims.y);
  };

  auto pack_in_window = [](Widget::Ptr w, sf::FloatRect alloc) -> Window::Ptr {
    auto window = Window::Create(Window::Style::BACKGROUND);
    auto box = Box::Create();
    box->Pack(w);
    window->Add(box);
    window->SetAllocation(alloc);
    return window;
  };

  // add proceed button
  auto b_proceed = Button::Create("PROCEED");
  b_proceed->GetSignal(Widget::OnLeftClick).Connect([this]() {
    done = true;
    accept = true;
  });

  auto w_proceed = pack_in_window(b_proceed, get_rect(0.3, 0.9, 0.4, 0.1));
  Add(w_proceed);

  // right label
  Box::Ptr right_panel = Box::Create(Box::Orientation::VERTICAL, 10);

  // choice guis
  Button::Ptr b_research = Button::Create("Research");
  bind_ppc(b_research, [this]() { reset_qw(research_gui()); });

  Button::Ptr b_military = Button::Create("Military");
  bind_ppc(b_military, [this]() {
    list<solar::ptr> solars;
    for (auto sid : g->selected_specific<solar>()) solars.push_back(g->get_specific<solar>(sid));
    if (solars.size()) reset_qw(military_gui(solars));
  });

  Button::Ptr b_governor = Button::Create("Governor");
  bind_ppc(b_governor, [this]() {
    list<solar::ptr> solars;
    for (auto sid : g->selected_specific<solar>()) solars.push_back(g->get_specific<solar>(sid));
    if (solars.size()) reset_qw(governor_gui(solars));
  });

  right_panel->Pack(b_research, true, false);
  right_panel->Pack(b_military, true, false);
  right_panel->Pack(b_governor, true, false);

  // hover info
  hover_label = Label::Create("Empty space");
  log_panel = Box::Create(Box::Orientation::VERTICAL);

  right_panel->Pack(graphics::wrap_in_scroll2(log_panel, 0.26 * desktop_dims.x, 0.3 * desktop_dims.y));
  right_panel->Pack(graphics::wrap_in_scroll2(hover_label, 0.26 * desktop_dims.x, 0.3 * desktop_dims.y));

  auto info_rect = sf::FloatRect(0.71 * desktop_dims.x, 20, 0.28 * desktop_dims.x, desktop_dims.y - 25);
  right_panel->SetAllocation(info_rect);
  auto w_info = pack_in_window(right_panel, info_rect);
  Add(w_info);

  LoadThemeFromFile("main.theme") || LoadThemeFromFile(utility::root_path + "/main.theme");
}

void main_interface::push_log(list<string> log) {
  static list<string> queue;
  int max_size = 40;

  for (auto v : log) queue.push_front(v);
  while (queue.size() > max_size) queue.pop_back();

  log_panel->RemoveAll();
  for (auto v : queue) {
    log_panel->Pack(Label::Create(v));
  }
}

void main_interface::bind_ppc(Widget::Ptr w, function<void(void)> f) {
  w->GetSignal(Widget::OnLeftClick).Connect([this, f]() {
    post_process.push_back(f);
  });
}

// let sfg::Desktop handle event, then run post processing callbacks
// added by children's event handlers
void main_interface::HandleEvent(const sf::Event &event) {
  Desktop::HandleEvent(event);
  for (auto f : post_process) f();
  post_process.clear();
}

research::data main_interface::get_research() {
  return g->players[g->self_id].research_level;
}

research::data *main_interface::access_research() {
  return &(g->players[g->self_id].research_level);
}

void main_interface::reset_qw(Widget::Ptr w) {
  clear_qw();
  if (w == 0) return;

  query_window = w;
  Add(query_window);
}

void main_interface::clear_qw() {
  if (query_window) Remove(query_window);
  query_window = 0;
}
