#include <vector>

#include "research.h"
#include "cost.h"

using namespace std;
using namespace st3;
using namespace research;
using namespace cost;

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
    s.damage_solar = 0;
    s.damage_ship = 0;
    s.accuracy = 0;
    s.load_time = 100;
    s.load = 0;

    a = s;
    a.speed = 2;
    a.vision = 100;
    a.ship_class = keywords::key_scout;
    a.damage_ship = 0.2;
    a.accuracy = 0.3;
    buf[a.ship_class] = a;

    a = s;
    a.hp = 2;
    a.interaction_radius = 20;
    a.ship_class = keywords::key_fighter;
    a.damage_solar = 1;
    a.damage_ship = 2;
    a.accuracy = 0.7;
    a.load_time = 30;
    buf[a.ship_class] = a;

    a = s;
    a.ship_class = keywords::key_bomber;
    a.damage_solar = 5;
    a.accuracy = 0.8;
    buf[a.ship_class] = a;

    a = s;
    a.speed = 0.5;
    a.hp = 2;
    a.ship_class = keywords::key_colonizer;
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
    x.accuracy = 0.5;
    x.load_time = 100;
    x.load = 0;

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
