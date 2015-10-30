#include "cost.h"

using namespace std;
using namespace st3;
using namespace cost;

hm_t<string, sector_cost> st3::cost::sector; 
hm_t<string, vmap> st3::cost::ship;
hm_t<string, vmap> st3::cost::defense;
hm_t<string, vmap> st3::cost::research;

void st3::cost::initialize(){
  
  // ships
  
  // scout
  ship["scout"]["metals"] = 1;
  ship["scout"]["organics"] = 1;
  ship["scout"]["time"] = 1;
  
  // fighter
  ship["fighter"]["metals"] = 2;
  ship["fighter"]["organics"] = 2;
  ship["fighter"]["time"] = 2;
  
  // bomber
  ship["bomber"]["metals"] = 6;
  ship["bomber"]["organics"] = 4;
  ship["bomber"]["time"] = 6;
  
  // colonizer
  ship["colonizer"]["metals"] = 3; 
  ship["colonizer"]["organics"] = 4;
  ship["colonizer"]["water"] = 3;
  ship["colonizer"]["time"] = 4;

  // infrastructure

  sector["agriculture"]["space"] = 1;
  sector["agriculture"]["water"] = 1;
  sector["agriculture"]["organics"] = 1;
  sector["agriculture"]["time"] = 1;  

  sector["infrastructure"]["metals"] = 2;
  sector["infrastructure"]["organics"] = 1;
  sector["infrastructure"]["time"] = 1;

  sector["mining"]["metals"] = 2;
  sector["mining"]["organics"] = 1;
  sector["mining"]["time"] = 1;

  ...
}
