#include <cstdlib>
#include <string>
#include <iostream>
#include <unordered_map>
#include <set>
#include <utility>

#include "command_selector.h"
#include "command_gui.h"
#include "utility.h"
#include "graphics.h"
#include "solar.h"

using namespace std;
using namespace st3;

command_gui::command_gui(command_selector::ptr c, game_data *g){
  sf::FloatRect bounds = interface::main_interface::qw_allocation;
  auto table = sfg::Table::Create();
  
  // setup command gui using ready ships from source plus ships
  // already selected for this command (they will not be listed as
  // ready)
  set<combid> ready_ships = g -> get_ready_ships(c -> source);
  ready_ships += c -> ships;

  hm_t<string, set<combid> > by_class;

  for (auto sid : ready_ships) {
    ship::ptr s = g -> get_ship(sid);
    by_class[s -> class_id].insert(s -> id);
  }

  auto setup_class = [g, c, table] (string class_id, set<combid> sids) {
    auto box = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 10);

    auto inner_table = [g, c, box] (set<combid> ships) -> sfg::Table::Ptr {
      auto tab = sfg::Table::Create();
      int n = ships.size();
      int rows = ceil(sqrt(n));
      int cols = ceil(n / rows);
      int w = ?;
      int h = ?;

      int i = 0;
      for (auto sid : ships) {
	ship::ptr s = g -> get_ship(sid);
	sfg::Button::Ptr b = graphics::ship_button(s -> ship_class, w, h, col);
	// todo: set b event handler

	int r = i / cols;
	int c = i % cols;
	int opt_x = sfg::Table::FILL | sfg::Table::EXPAND;
	int opt_y = sfg::Table::FILL;
	point padding(5, 5);
	tab -> Attach(b, sf::Rect<sf::Uint32>( c, r, 1, 1 ), opt_x, opt_y, padding);
	i++;
      }

      return tab;
    };
  };

}
