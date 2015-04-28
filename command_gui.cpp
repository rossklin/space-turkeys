#include <cstdlib>
#include <string>
#include <iostream>
#include <unordered_map>
#include <set>
#include <utility>

#include "command_gui.h"
#include "utility.h"
#include "graphics.h"

using namespace std;
using namespace st3;

command_gui::command_gui(idtype cid, sf::RenderWindow &window, hm_t<idtype, ship> s, std::set<idtype> prealloc, point p, sf::Color c) : w(window){
  point box_dims;
  comid = cid;
  color = c;
  done = false;
  bounds.left = p.x;
  bounds.top = p.y;
  bounds.width = 200;
  bounds.height = 150;
  offset = 10;
  box_dims.x = bounds.width / 2 - 2 * offset;
  box_dims.y = bounds.height - 2 * offset;
  cache_bounds = sf::FloatRect(p.x + offset, p.y + offset, box_dims.x, box_dims.y);
  alloc_bounds = sf::FloatRect(p.x + bounds.width / 2 + offset, p.y + offset, box_dims.x, box_dims.y);
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
    grid_size.x = sqrt(ships.size() / 2);
    grid_size.y = ships.size() / grid_size.x + !!(ships.size() % grid_size.x);

    symbol_dims.x = box_dims.x / grid_size.x;
    symbol_dims.y = box_dims.y / grid_size.y;
  }else{
    grid_size.x = grid_size.y = 0;
    symbol_dims.x = symbol_dims.y = 1;
  }
}

void command_gui::draw(){
  // draw root box
  sf::RectangleShape r = utility::build_rect(bounds);
  r.setFillColor(sf::Color(20, 180, 50, 80));
  r.setOutlineColor(sf::Color(80, 120, 240, 200));
  r.setOutlineThickness(3);
  w.draw(r);

  // draw inner boxes
  r = utility::build_rect(cache_bounds);
  w.draw(r);
  r = utility::build_rect(alloc_bounds);
  w.draw(r);

  // draw ship symbols
  int count = 0;
  int row, col;
  for (auto x : cached){
    row = count / grid_size.x;
    col = count % grid_size.x;
    ship s = ships[x];
    s.position.x = cache_bounds.left + col * symbol_dims.x + 0.5 * s.interaction_radius;
    s.position.y = cache_bounds.top + row * symbol_dims.y + 0.5 * s.interaction_radius;
    s.angle = 0;
    graphics::draw_ship(w, s, color);

    cache_ids[count] = x;
    count++;
  }

  count = 0;
  for (auto x : allocated){
    row = count / grid_size.x;
    col = count % grid_size.x;
    ship s = ships[x];
    s.position.x = alloc_bounds.left + col * symbol_dims.x + 0.5 * s.interaction_radius;
    s.position.y = alloc_bounds.top + row * symbol_dims.y + 0.5 * s.interaction_radius;
    s.angle = 0;
    graphics::draw_ship(w, s, color);

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
    w.draw(r);
  }
}

void command_gui::move_ship(idtype id){
  if (cached.count(id)){
    cached.erase(id);
    allocated.insert(id);
    cout << "command_gui: moved " << id << " to allocated" << endl;
  }else if (allocated.count(id)){
    allocated.erase(id);
    cached.insert(id);
    cout << "command_gui: moved " << id << " to cached" << endl;
  }
}

bool command_gui::handle_event(sf::Event e){
  point p;
  int i;

  switch (e.type){
  case sf::Event::MouseButtonReleased:
    if (e.mouseButton.button == sf::Mouse::Left){
      p = w.mapPixelToCoords(sf::Vector2i(e.mouseButton.x, e.mouseButton.y));
      if ((i = get_cache_index(p)) > -1){
	move_ship(cache_ids[i]);
      }else if ((i = get_alloc_index(p)) > -1){
	move_ship(alloc_ids[i]);
      }
    }

    return bounds.contains(p);
  case sf::Event::MouseButtonPressed:
    p = w.mapPixelToCoords(sf::Vector2i(e.mouseButton.x, e.mouseButton.y));
    return bounds.contains(p);
  case sf::Event::MouseMoved:
    p = w.mapPixelToCoords(sf::Vector2i(e.mouseMove.x, e.mouseMove.y));
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

sf::Vector2i command_gui::coord2index(point r, point p){
  point delta = p - r;
  return sf::Vector2i(delta.x / symbol_dims.x, delta.y / symbol_dims.y);
}

int command_gui::get_cache_index(point p){
  sf::Vector2i where = coord2index(point(cache_bounds.left, cache_bounds.top), p);

  if (where.x >= 0 && where.x < grid_size.x && where.y >= 0 && where.y < grid_size.y){
    int idx = where.x + grid_size.x * where.y;
    if (idx < cached.size()){
      return idx;
    }
  }
  
  return -1;
}

int command_gui::get_alloc_index(point p){
  sf::Vector2i where = coord2index(point(alloc_bounds.left, alloc_bounds.top), p);

  if (where.x >= 0 && where.x < grid_size.x && where.y >= 0 && where.y < grid_size.y){
    int idx = where.x + grid_size.x * where.y;
    if (idx < allocated.size()){
      return idx;
    }
  }
  
  return -1;
}
