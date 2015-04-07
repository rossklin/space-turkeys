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

entity_selector::entity_selector(idtype i, bool o){
  id = i;
  owned = o;
}

// ****************************************
// SOLAR SELECTOR
// ****************************************

solar_selector::solar_selector( solar *s, idtype i, bool o) : entity_selector(i, o){
  data = s;
}

bool solar_selector::contains_point(point p, float &d){
  d = l2norm(p - data -> position);
  return d < data -> radius;
}

source_t solar_selector::command_source(){
  source_t s;
  s.id = id;
  s.type = source_t::SOLAR;
  return s;
}

// ****************************************
// FLEET SELECTOR
// ****************************************

fleet_selector::fleet_selector( fleet *s, idtype i, bool o) : entity_selector(i, o){
  data = s;
}

bool fleet_selector::contains_point(point p, float &d){
  d = l2norm(p - data -> position);
  return d < data -> radius;
}


source_t fleet_selector::command_source(){
  source_t s;
  s.id = id;
  s.type = source_t::FLEET;
  return s;
}

// ****************************************
// COMMAND SELECTOR
// ****************************************

command_selector::command_selector(command com, point s, point d){
  c = com;
  from = s;
  to = d;
}

bool command_selector::contains_point(point p, float &d){
  d = dpoint2line(p, from, to);
  // todo: command size?
  return true;
}
