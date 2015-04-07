#include "selector.h"
#include "solar.h"
#include "fleet.h"

using namespace std;
using namespace st3;
using namespace st3::client;

float l2norm(point p);
float dpoint2line(point p, point from, point to);

// ****************************************
// ENTITY SELECTOR
// ****************************************

entity_selector::entity_selector(idtype i, bool o, point p){
  id = i;
  owned = o;
  position = p;
  area_selectable = true;
}

// ****************************************
// SOLAR SELECTOR
// ****************************************

solar_selector::solar_selector(idtype i, bool o, point p, float r) : entity_selector(i, o, p){
  radius = r;
}

bool solar_selector::contains_point(point p, float &d){
  d = l2norm(p - position);
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
  d = l2norm(p - position);
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
  d = dpoint2line(p, from, position);

  // todo: command size?
  return true;
}

source_t command_selector::command_source(){
  return identifier::make(identifier::command, id);
}
