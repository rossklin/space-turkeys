#include "choice.h"
#include "solar.h"
#include "utility.h"

#include <list>
#include <iostream>
#include <cstdlib>

using namespace std;
using namespace st3;

solar build_solar(){
  solar s;

  s.dt = 0.1;
  s.population = 1000;
  s.happiness = 1;
  s.research = 0;
    
  s.water = 1000;
  s.space = 1000;
  s.ecology = 1;

  for (auto v : cost::keywords::resource){
    s.resource[v].available = 1000;
    s.resource[v].storage = 0;
  }

  for (auto v : cost::keywords::sector)
    s.sector[v] = 0;

  return s;
}

int main(int argc, char **argv){
  auto ctable = choice::c_solar::template_table();
  int entities = 10;
  int steps = 4000;
  string sc = "military expansion";

  if (argc > 1) sc = argv[1];
  if (argc > 2) entities = atoi(argv[2]);
  if (argc > 3) steps = atoi(argv[3]);

  cout << "entity, step, population, happiness, ecology, water, space, culture, resource, storage, ships," << endl;

  for (int i = 0; i < entities; i++){
    solar s = build_solar();
    choice::c_solar c = ctable[sc];

    // c.allocation[cost::keywords::key_culture] = 1;
    // c.allocation[cost::keywords::key_expansion] = 1;
    // c.allocation[cost::keywords::key_mining] = 1;
    // c.allocation[cost::keywords::key_military] = 0;
    // c.allocation[cost::keywords::key_research] = 0;
    
    // c.expansion[cost::keywords::key_culture] = 1;
    // c.expansion[cost::keywords::key_military] = 0;
    // c.expansion[cost::keywords::key_mining] = 0;
    // c.expansion[cost::keywords::key_research] = 0;

    // c.mining[cost::keywords::key_gases] = 1;
    // c.mining[cost::keywords::key_metals] = 1;
    // c.mining[cost::keywords::key_organics] = 1;

    // c.military.c_ship[cost::keywords::key_fighter] = 1;

    s.choice_data = c;
    
    for (int j = 0; j < steps; j++){
      s.dynamics();
      cout << i << ", " << j << ", " << s.population << ", " << s.happiness << ", " << s.ecology << ", " << s.water_status() << ", " << s.space_status() << ", " << s.sector[cost::keywords::key_culture] << ", " << fmin(s.resource["gases"].available, fmin(s.resource["metals"].available, s.resource["organics"].available)) << ", " << fmin(s.resource["gases"].storage, fmin(s.resource["metals"].storage, s.resource["organics"].storage)) << ", " << s.fleet_growth[cost::keywords::key_fighter] << "," << endl;
    }
  }

  return 0;
}
