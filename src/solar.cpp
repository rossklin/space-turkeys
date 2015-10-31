#include <vector>
#include <iostream>

#include "research.h"
#include "solar.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace solar;

float solar::resource_constraint(vmap r){
  float m = INFINITY;

  for (auto x : r) {
    if (r.second > 0) m = fmin(m, resource[r.first] / r.second);
  }

  return m;
}

st3::solar::solar::solar(){}

string st3::solar::solar::get_info(){
  stringstream ss;
  // ss << "fleet_growth: " << fleet_growth << endl;
  // ss << "new_research: " << new_research << endl;
  // ss << "industry: " << industry << endl;
  // ss << "resource storage: " << resource_storage << endl;
  ss << "pop: " << population << "(" << happieness << ")" << endl;
  // ss << "resource: " << resource << endl;
  ss << "ships: " << ships.size() << endl;
  ss << "defence: " << defense_current << "(" << defense_capacity << ")" << endl;
  return ss.str();
}

float st3::solar::solar::pop_increment(research const &r, float allocated){
  float farmers = fmin(sector_capacity["agriculture"], allocated);
  float food_cap = r.population.food_rate() * farmers;

  float feed = r.population.feed_rate() * (food_cap - population);
  float crowding = r.crowding_rate() * (housing - population);

  cout << "pop: " << population << ", happy: " << happieness << ", farmers: " << farmers << ", cap: " << food_cap << ", feed: " << feed << ", crowding: " << crowding << endl;

  return (r.birth_rate() - r.death_rate() - crowding) * population + feed;
}

float st3::solar::solar::farmers_for_growthratio(float q, research const &r){
  ...
}
