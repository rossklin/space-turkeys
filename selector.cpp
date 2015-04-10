#include <iostream>

#include "selector.h"
#include "solar.h"
#include "fleet.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace st3::client;

// ****************************************
// ENTITY SELECTOR
// ****************************************

entity_selector::entity_selector(idtype i, bool o, point p){
  id = i;
  owned = o;
  position = p;
  area_selectable = true;
  selected = false;
}

bool entity_selector::inside_rect(sf::FloatRect r){
  return r.contains(position);
}

// ****************************************
// SOLAR SELECTOR
// ****************************************

solar_selector::solar_selector(idtype i, bool o, point p, float r) : entity_selector(i, o, p){
  radius = r;
}

bool solar_selector::contains_point(point p, float &d){
  d = utility::l2norm(p - position);
  return d < radius;
}

source_t solar_selector::command_source(){
  return identifier::make(identifier::solar, id);
}

// ****************************************
// FLEET SELECTOR
// ****************************************

fleet_selector::fleet_selector(idtype i, bool o, point p, float r) : entity_selector(i, o, p){
  radius = r;
}

bool fleet_selector::contains_point(point p, float &d){
  d = utility::l2norm(p - position);
  return d < radius;
}


source_t fleet_selector::command_source(){
  return identifier::make(identifier::fleet, id);
}

// ****************************************
// COMMAND SELECTOR
// ****************************************

command_selector::command_selector(idtype i, point s, point d) : entity_selector(i, true, d){
  from = s;
  area_selectable = false;
}

bool command_selector::contains_point(point p, float &d){
  d = utility::dpoint2line(p, from, position);

  // todo: command size?
  return d < 5;
}

source_t command_selector::command_source(){
  return identifier::make(identifier::command, id);
}

point command_selector::get_to(){
  return position;
}

point command_selector::get_from(){
  return from;
}
