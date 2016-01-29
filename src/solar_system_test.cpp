#include "choice.h"
#include "solar.h"
#include "utility.h"

#include <list>
#include <iostream>
#include <cstdlib>

using namespace std;
using namespace st3;

solar::solar build_solar(){
  solar::solar s;

  s.population = 1000 * utility::random_uniform();
  s.happiness = 1;
  s.research = 0;
    
  s.water = 1000 * utility::random_uniform();
  s.space = 1000 * utility::random_uniform();
  s.ecology = utility::random_uniform();

  for (auto v : cost::keywords::resource)
    s.resource[v].available = 1000 * utility::random_uniform();
  return s;
}

int main(int argc, char **argv){
  float dt = 0.1;
  auto ctable = choice::c_solar::template_table();
  int entities = 10;
  int steps = 1000;

  cout << "entity, step, population, happiness, ecology, water, space," << endl;

  for (int i = 0; i < entities; i++){
    solar::solar s = build_solar();
    choice::c_solar c = ctable["basic growth"];
    
    for (int j = 0; j < steps; j++){
      s = s.dynamics(c, dt);
      cout << i << ", " << j << ", " << s.population << ", " << s.happiness << ", " << s.ecology << ", " << s.water_status() << ", " << s.space_status() << "," << endl;
    }
  }

  return 0;
}
