#include <vector>

#include "research.h"

using namespace std;
using namespace st3;
using namespace research;

cost::ship_allocation<ship>& research::ship_templates(){
  static bool init = false;
  static cost::ship_allocation<ship> buf;

  if (!init){
    init = true;
    
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
    buf[a.ship_class] = a;

    a = s;
    a.hp = 2;
    a.interaction_radius = 20;
    a.ship_class = "fighter";
    buf[a.ship_class] = a;

    a = s;
    a.ship_class = "bomber";
    buf[a.ship_class] = a;

    a = s;
    a.speed = 0.5;
    a.hp = 2;
    a.ship_class = "colonizer";
    buf[a.ship_class] = a;

    buf.confirm_content(cost::keywords::ship);
  }

  return buf;
}

cost::turret_allocation<turret> &research::turret_templates(){
  static bool init = false;
  static cost::turret_allocation<turret> buf;

  if (!init){
    init = true;
    turret x,a;

    x.range = 50;
    x.hp = 1;
    x.vision = 100;
    x.damage = 1;

    a = x;
    a.turret_class = cost::keywords::key_rocket_turret;
    buf[a.turret_class] = a;

    a = x;
    a.damage = 0;
    a.vision = 200;
    a.turret_class = cost::keywords::key_radar_turret;
    buf[a.turret_class] = a;

    buf.confirm_content(cost::keywords::turret);
  }

  return buf;
}

data::data(){
  x = "research level 0";
}

ship data::build_ship(string c){
  ship s = ship_templates()[c];

  // todo: apply research boosts

  return s;
}

turret data::build_turret(string v){
  turret t = turret_templates()[v];

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
