#include <vector>

#include "research.h"

using namespace std;
using namespace st3;
using namespace research;

cost::ship_allocation<ship> &research::ship_templates;
cost::turret_allocation<turret> &research::turret_templates;

void research::initialize(){
  
  auto ship_templates_b = new cost::ship_allocation<ship>();
  auto turret_templates_b = new cost::turret_allocation<turret>();
  ship_templates = *ship_templates_b;
  turret_templates = *turret_templates_b;

  ship s, a;
  s.speed = 1;
  s.vision = 50;
  s.hp = 1;
  s.interaction_radius = 10;
  s.fleet_id = -1;
  s.ship_class = "";
  s.was_killed = false;

  a = s;
  a.speed = 2;
  a.vision = 100;
  a.ship_class = "scout";
  ship_templates[a.ship_class] = a;

  a = s;
  a.hp = 2;
  a.interaction_radius = 20;
  a.ship_class = "fighter";
  ship_templates[a.ship_class] = a;

  a = s;
  a.ship_class = "bomber";
  ship_templates[a.ship_class] = a;

  a = s;
  a.speed = 0.5;
  a.hp = 2;
  a.ship_class = "colonizer";
  ship_templates[a.ship_class] = a;

  ship_templates.confirm_content(cost::keywords::ship);

  turret x, buf;

  x.range = 50;
  x.hp = 1;
  x.vision = 100;
  x.damage = 1;

  buf = x;
  buf.turret_class = cost::keywords::key_rocket_turret;
  turret_templates[buf.turret_class] = buf;

  buf = x;
  buf.damage = 0;
  buf.vision = 200;
  buf.turret_class = cost::keywords::key_radar_turret;
  turret_templates[buf.turret_class] = buf;

  turret_templates.confirm_content(cost::keywords::turret);
}

void research::cleanup(){
  delete &ship_templates;
  delete &turret_templates;
}

data::data(){
  x = "research level 0";
}

ship data::build_ship(string c){
  ship s = ship_templates[c];

  // todo: apply research boosts

  return s;
}

turret data::build_turret(string v){
  turret t = turret_templates[v];

  // todo: apply research boosts

  return t;
}

void data::colonize(solar::solar *s){
  s -> population = 100;
  s -> happiness = 1;
}

int data::colonizer_population(){
  return 100;
}

void data::develope(float d){
  x = "research level " + to_string(d);
}
