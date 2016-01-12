#include <vector>

#include <SFGUI/SFGUI.hpp>
#include <SFGUI/Widgets.hpp>
#include <SFML/Graphics.hpp>

#include "research.h"

using namespace std;
using namespace st3;
using namespace research;
using namespace sfg;

cost::ship_allocation<ship> research::ship_templates;

void initialize(){
  ship s, a;
  s.speed = 1;
  s.vision = 50;
  s.hp = 1;
  s.interaction_radius = 10;
  s.fleet_id = -1;
  s.ship_class = "";
  s.was_killed = false;

  ship_templates.setup(cost::keywords::ship);

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
}

data::data(){
  x = "this is a research object";
}

ship data::build_ship(string c){
  ship s = ship_templates[c];

  // todo: apply research boosts

  return s;
}
