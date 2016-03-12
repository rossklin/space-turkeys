#include <cstdlib>
#include <string>
#include <iostream>
#include <unordered_map>
#include <set>
#include <utility>

#include "command_gui.h"
#include "utility.h"
#include "graphics.h"
#include "solar.h"

using namespace std;
using namespace st3;

float command_gui::table_width = 0;
float command_gui::table_height = 0;

command_gui::command_gui(idtype cid, window_t *window, hm_t<combid, ship> s, set<combid> prealloc, point dims, sf::Color c, string hstring){
  float margin = 10;
  point p(margin, margin);
  w = window;
  comid = cid;
  header_string = hstring;
  table_width = 0.4 * (dims.x - 2 * (padding + margin));
  table_height = (dims.y - header_height - 2 * (padding + margin)) / cost::keywords::ship.size();
  bounds = sf::FloatRect(p.x, p.y, table_width + 2 * padding, dims.y - 2 * margin);

  tables.resize(cost::keywords::ship.size());
  int i = 0;
  for (auto v : cost::keywords::ship){
    // collect data for this ship class table
    hm_t<combid, ship> sbuf;
    set<combid> pbuf;
    for (auto x : s){
      if (x.second.ship_class == v) {
	sbuf[x.first] = x.second;
	if (prealloc.count(x.first)) pbuf.insert(x.first);
      }
    }
    point x = p + point(padding, padding + header_height + i * table_height);

    tables[i] = command_table(w, sbuf, pbuf, x, c);
    i++;
  }
}

bool command_gui::handle_event(sf::Event e){
  bool res = false;
  cached.clear();
  allocated.clear();

  for (auto &t : tables){
    res |= t.handle_event(e);
    cached += t.cached;
    allocated += t.allocated;
  }

  return res;
}

void command_gui::draw(){
  // draw root box
  sf::RectangleShape r = graphics::build_rect(bounds);
  r.setFillColor(sf::Color(20, 180, 50, 40));
  r.setOutlineColor(sf::Color(80, 120, 240, 200));
  r.setOutlineThickness(1);
  w -> draw(r);

  // draw header
  sf::Text text;
  text.setFont(graphics::default_font); 
  text.setCharacterSize(0.5 * header_height);
  text.setColor(sf::Color(200,200,200));
  text.setString(header_string);

  // scale text to fit and center it
  float text_margin = 2;
  float text_bound = bounds.width - 2 * text_margin;
  point text_dims(text.getLocalBounds().width, text.getLocalBounds().height);
  float rescale = (text_bound) / fmax(text_bound, text_dims.x);
  text.setScale(point(rescale, rescale));
  text_dims.x *= rescale;
  text_dims.y *= rescale;
  text.setPosition(point(bounds.left + text_margin + (text_bound - text_dims.x) / 2, bounds.top + (header_height - text_dims.y) / 2));
  w -> draw(text);

  // draw tables
  for (auto &t : tables) t.draw();
}

command_table::command_table(){}

command_table::command_table(window_t *window, hm_t<combid, ship> s, set<combid> prealloc, point p, sf::Color c){
  point box_dims;
  inner_padding = 20;
  outer_padding = 5;

  w = window;
  color = c;
  done = false;
  bounds.left = p.x;
  bounds.top = p.y;
  bounds.width = command_gui::table_width;
  bounds.height = command_gui::table_height;
  box_dims.x = bounds.width / 2 - inner_padding - outer_padding;
  box_dims.y = bounds.height - 2 * outer_padding;
  cache_bounds = sf::FloatRect(p.x + outer_padding, p.y + outer_padding, box_dims.x, box_dims.y);
  alloc_bounds = sf::FloatRect(p.x + bounds.width / 2 + inner_padding, p.y + outer_padding, box_dims.x, box_dims.y);
  hover_side = -1;

  cache_ids.resize(s.size());
  alloc_ids.resize(s.size());

  for (auto x : s){
    cached.insert(x.first);
  }

  for (auto x : prealloc){
    cached.erase(x);
    allocated.insert(x);
  }

  ships = s;

  if (ships.size() > 0){
    grid_size.x = ceil(sqrt(ships.size() / (float)2));
    grid_size.y = ships.size() / grid_size.x + !!(ships.size() % grid_size.x);

    symbol_dims.x = box_dims.x / grid_size.x;
    symbol_dims.y = box_dims.y / grid_size.y;
  }else{
    grid_size.x = grid_size.y = 0;
    symbol_dims.x = symbol_dims.y = 1;
  }

  // fill arrows setup
  float arrow_top = bounds.top + outer_padding;
  float arrow_mid = bounds.top + bounds.height / 2;
  float arrow_bottom = bounds.top + bounds.height - outer_padding;
  float arrow_height = (bounds.height - 2 * outer_padding) / 4;
  float arrow_left = cache_bounds.left + cache_bounds.width + 2;
  float arrow_right = alloc_bounds.left - 2;
  
  fa_arrow.setPointCount(3);
  fa_arrow.setPoint(0, point(arrow_left, arrow_top));
  fa_arrow.setPoint(1, point(arrow_right, arrow_top + arrow_height));
  fa_arrow.setPoint(2, point(arrow_left, arrow_mid));
  fa_arrow.setFillColor(sf::Color(200, 210, 180, 100));
  fa_arrow.setOutlineColor(sf::Color(40, 250, 80, 150));
  fa_arrow.setOutlineThickness(-3);

  fc_arrow.setPointCount(3);
  fc_arrow.setPoint(0, point(arrow_right, arrow_mid));
  fc_arrow.setPoint(1, point(arrow_left, arrow_mid + arrow_height));
  fc_arrow.setPoint(2, point(arrow_right, arrow_bottom));
  fc_arrow.setFillColor(sf::Color(200, 210, 180, 100));
  fc_arrow.setOutlineColor(sf::Color(250, 40, 80, 150));
  fc_arrow.setOutlineThickness(-3);

}

void command_table::draw(){
  // draw root box
  sf::RectangleShape r = graphics::build_rect(bounds);
  r.setFillColor(sf::Color(20, 180, 50, 40));
  r.setOutlineColor(sf::Color(80, 120, 240, 200));
  r.setOutlineThickness(1);
  w -> draw(r);

  // draw inner boxes
  r = graphics::build_rect(cache_bounds);
  r.setFillColor(sf::Color(40, 120, 80, 40));
  r.setOutlineColor(sf::Color(80, 120, 240, 200));
  r.setOutlineThickness(1);
  w -> draw(r);
  r = graphics::build_rect(alloc_bounds);
  r.setFillColor(sf::Color(40, 120, 80, 40));
  r.setOutlineColor(sf::Color(80, 120, 240, 200));
  r.setOutlineThickness(1);
  w -> draw(r);

  // draw allocate/cache all symbols
  w -> draw(fa_arrow);
  w -> draw(fc_arrow);

  // draw ship symbols
  int count = 0;
  float ship_scale = (fmin(symbol_dims.x, symbol_dims.y) - 4) / 6;
  int row, col;
  for (auto x : cached){
    row = count / grid_size.x;
    col = count % grid_size.x;
    ship s = ships[x];
    s.position.x = cache_bounds.left + (col + 0.5) * symbol_dims.x;
    s.position.y = cache_bounds.top + (row + 0.5) * symbol_dims.y;
    s.angle = 0;
    graphics::draw_ship(*w, s, color, ship_scale);

    cache_ids[count] = x;
    count++;
  }

  count = 0;
  for (auto x : allocated){
    row = count / grid_size.x;
    col = count % grid_size.x;
    ship s = ships[x];
    s.position.x = alloc_bounds.left + (col + 0.5) * symbol_dims.x;
    s.position.y = alloc_bounds.top + (row + 0.5) * symbol_dims.y;
    s.angle = 0;
    graphics::draw_ship(*w, s, color, ship_scale);

    alloc_ids[count] = x;
    count++;
  }

  // indicate hover
  if (hover_side > -1){
    row = hover_index / grid_size.x;
    col = hover_index % grid_size.x;

    if (hover_side == 0){
      r.setPosition(cache_bounds.left + col * symbol_dims.x, cache_bounds.top + row * symbol_dims.y);
    }else{
      r.setPosition(alloc_bounds.left + col * symbol_dims.x, alloc_bounds.top + row * symbol_dims.y);
    }

    r.setSize(symbol_dims);

    r.setFillColor(sf::Color(250,250,250,25));
    r.setOutlineThickness(0);
    w -> draw(r);
  }
}

void command_table::move_ship(combid id){
  if (cached.count(id)){
    cached.erase(id);
    allocated.insert(id);
    cout << "command_table: moved " << id << " to allocated" << endl;
  }else if (allocated.count(id)){
    allocated.erase(id);
    cached.insert(id);
    cout << "command_table: moved " << id << " to cached" << endl;
  }
}

bool command_table::handle_event(sf::Event e){
  point p;
  int i;

  switch (e.type){
  case sf::Event::MouseButtonReleased:
    if (e.mouseButton.button == sf::Mouse::Left){
      p = w -> mapPixelToCoords(sf::Vector2i(e.mouseButton.x, e.mouseButton.y));
      
      if ((i = get_cache_index(p)) > -1){
	cout << "found cache index: " << i << endl;
	move_ship(cache_ids[i]);
      }else if ((i = get_alloc_index(p)) > -1){
	cout << "found alloc index: " << i << endl;
	move_ship(alloc_ids[i]);
      }else if (fa_arrow.getLocalBounds().contains(p)){
	cout << "filling allocated" << endl;
	allocated += cached;
	cached.clear();
      }else if (fc_arrow.getLocalBounds().contains(p)){
	cout << "filling cached" << endl;
	cached += allocated;
	allocated.clear();
      }
    }

    return bounds.contains(p);
  case sf::Event::MouseButtonPressed:
    p = w -> mapPixelToCoords(sf::Vector2i(e.mouseButton.x, e.mouseButton.y));
    return bounds.contains(p);
  case sf::Event::MouseMoved:
    p = w -> mapPixelToCoords(sf::Vector2i(e.mouseMove.x, e.mouseMove.y));
    if ((i = get_cache_index(p)) > -1){
      hover_index = i;
      hover_side = 0;
    }else if ((i = get_alloc_index(p)) > -1){
      hover_index = i;
      hover_side = 1;
    }else{
      hover_side = -1;
    }
    break;
  }
  
  return false;
}

sf::Vector2i command_table::coord2index(point r, point p){
  point delta = p - r;
  return sf::Vector2i(floor(delta.x / symbol_dims.x), floor(delta.y / symbol_dims.y));
}

int command_table::get_cache_index(point p){
  sf::Vector2i where = coord2index(point(cache_bounds.left, cache_bounds.top), p);

  if (where.x >= 0 && where.x < grid_size.x && where.y >= 0 && where.y < grid_size.y){
    int idx = where.x + grid_size.x * where.y;
    if (idx < cached.size()){
      return idx;
    }
  }
  
  return -1;
}

int command_table::get_alloc_index(point p){
  sf::Vector2i where = coord2index(point(alloc_bounds.left, alloc_bounds.top), p);

  if (where.x >= 0 && where.x < grid_size.x && where.y >= 0 && where.y < grid_size.y){
    int idx = where.x + grid_size.x * where.y;
    if (idx < allocated.size()){
      return idx;
    }
  }
  
  return -1;
}
