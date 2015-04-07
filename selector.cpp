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

// utility stuff
float l2norm(point p){
  return sqrt(pow(p.x, 2) + pow(p.y, 2));
}

float point_angle(point p){
  if (p.x > 0){
    return atan(p.y / p.x);
  }else if (p.x < 0){
    return M_PI + atan(p.y / p.x);
  }else if (p.y > 0){
    return M_PI / 2;
  }else {
    return -M_PI / 2;
  }
}

float point_mult(point a, point b){
  return a.x * b.x + a.y * b.y;
}

point scale_point(point p, float a){
  return point(a * p.x, a * p.y);
}

float sproject(point a, point r){
  return point_mult(a,r) / point_mult(r,r);
}

// shortest distance between angles a and b
float angle_distance(float a, float b){
  float r;

  a = fmod(a, 2 * M_PI);
  b = fmod(a, 2 * M_PI);

  r = fabs(b - a);

  if (r > M_PI){
    r = 2 * M_PI - r;
  }

  return r;
}

// shortest distance between p and line from a to b
float dpoint2line(point p, point a, point b){
  if (angle_distance(point_angle(p - a), point_angle(b - a)) > PI/2){
    return l2norm(p - a);
  }else if (angle_distance(point_angle(p - b), point_angle(a - b)) > PI/2){
    return l2norm(p - b);
  }else{
    return l2norm(p - scale_point(b - a, sproject(p-a,b-a)) - a);
  }
}


