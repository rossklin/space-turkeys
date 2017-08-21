#include "choice.h"
#include "solar.h"
#include "utility.h"
#include "research.h"
#include "types.h"

#include <list>
#include <iostream>
#include <cstdlib>

using namespace std;
using namespace st3;

solar::ptr build_solar(){
  solar::ptr s = solar::create(point(0, 0), 1);

  s -> dt = 0.1;
  s -> population = 1000;
  s -> happiness = 1;
  s -> research_points = 0;
    
  s -> water = 1000;
  s -> space = 1000;
  s -> ecology = 1;

  for (auto v : keywords::resource){
    s -> available_resource[v] = 1000;
    s -> resource_storage[v] = 0;
  }

  return s;
}

int main(int argc, char **argv){
  int entities = 10;
  int steps = 4000;
  string gov = "culture";
  research::data rdata;

  if (argc > 1) gov = argv[1];
  if (argc > 2) entities = atoi(argv[2]);
  if (argc > 3) steps = atoi(argv[3]);

  hm_t<string, function<float(solar::ptr)> > stat;

  stat["population"] = [] (solar::ptr s) -> float {
    return s -> population / 10000;
  };

  stat["happiness"] = [] (solar::ptr s) -> float {
    return s -> happiness;
  };

  stat["environment"] = [] (solar::ptr s) -> float {
    return s -> ecology;
  };

  stat["crowding"] = [] (solar::ptr s) -> float {
    return s -> crowding_rate() / s -> population;
  };

  stat["resources"] = [] (solar::ptr s) -> float {
    cost::res_t res = s -> available_resource;
    res.add(s -> resource_storage);
    return fmin(res["gases"], fmin(res["metals"], res["organics"])) / 1000;
  };

  stat["ships"] = [] (solar::ptr s) -> float {
    return s -> fleet_growth.count() / 1000;
  };

  stat["culture"] = [] (solar::ptr s) -> float {
    return s -> compute_boost(keywords::key_culture) - 1;
  };

  stat["medicine"] = [] (solar::ptr s) -> float {
    return s -> compute_boost(keywords::key_medicine) - 1;
  };

  stat["ecology"] = [] (solar::ptr s) -> float {
    return s -> compute_boost(keywords::key_ecology) - 1;
  };

  cout << "entity, step, ";
  for (auto f : stat) cout << f.first << ", ";
  cout << endl;

  research::data r;
  server::silent_mode = true;
  for (int i = 0; i < entities; i++){
    solar::ptr s = build_solar();

    s -> research_level = &rdata;
    s -> choice_data.allocation = cost::sector_allocation::base_allocation();
    s -> choice_data.governor = gov;
    s -> next_ship = "fighter";
    
    for (int j = 0; j < steps; j++){
      s -> dynamics();

      // build facilities (hacked from solar.cpp)
      if (s -> choice_data.development.length() > 0) {
	string dev = s -> choice_data.development;
	if (s -> list_facility_requirements(dev, rdata).empty()) {
	  if (s -> develop(dev)) {
	    s -> choice_data.development.clear();
	  }
	}
      }

      cout << i << ", " << j << ", ";
      for (auto f : stat) cout << f.second(s) << ", ";
      cout << endl;
    }

    delete s;
  }

  return 0;
}
