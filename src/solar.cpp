#include <vector>
#include <iostream>

#include "research.h"
#include "solar.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace solar;

vector<vector<float> > st3::solar::ship_cost;
vector<vector<float> > st3::solar::industry_cost;
vector<float> st3::solar::ship_buildtime;
vector<string> st3::solar::work_names;
vector<vector<string> > st3::solar::sub_names;
vector<float> st3::solar::p3;

const float st3::solar::births_per_person = 2e-3;
const float st3::solar::deaths_per_person = 2e-3;
const float st3::solar::agriculture_limit_coefficient = 1e3;
const float st3::solar::feed_boost_coefficient = 2e-2;
const float st3::solar::crowding_rate = 1e-6;
const float st3::solar::food_per_person = 2;
const float st3::solar::fpp_per_research = 2e-2;
const float st3::solar::colonizer_population = 1e2;

void st3::solar::initialize(){
  ship_cost.resize(ship_num);
  ship_buildtime.resize(ship_num);
  industry_cost.resize(work_num);

  for (auto &x : ship_cost) x.resize(resource_num);
  
  // ships
  // scout
  ship_cost[ship_scout][resource_metal] = 1;
  ship_cost[ship_scout][resource_gas] = 1;
  ship_cost[ship_scout][resource_organics] = 1;
  ship_buildtime[ship_scout] = 10;

  // fighter
  ship_cost[ship_fighter][resource_metal] = 1;
  ship_cost[ship_fighter][resource_gas] = 2;
  ship_buildtime[ship_fighter] = 20;

  // bomber
  ship_cost[ship_bomber][resource_metal] = 3;
  ship_cost[ship_bomber][resource_gas] = 2;
  ship_buildtime[ship_bomber] = 30;

  // colonizer
  ship_cost[ship_colonizer][resource_metal] = 3;
  ship_cost[ship_colonizer][resource_gas] = 2;
  ship_cost[ship_colonizer][resource_organics] = 4;
  ship_buildtime[ship_colonizer] = 40;

  // industry
  // costs
  for (auto &x : industry_cost) x.resize(resource_num);

  industry_cost[work_expansion][resource_metal] = 0.4;
  industry_cost[work_expansion][resource_gas] = 0.1;
  industry_cost[work_expansion][resource_organics] = 0.1;
  industry_cost[work_ship][resource_metal] = 0.4;
  industry_cost[work_ship][resource_gas] = 0.2;
  industry_cost[work_research][resource_metal] = 0.1;
  industry_cost[work_research][resource_gas] = 0.4;
  industry_cost[work_research][resource_organics] = 0.4;
  industry_cost[work_resource][resource_metal] = 0.2;
  industry_cost[work_resource][resource_gas] = 0.1;
  industry_cost[work_resource][resource_organics] = 0.2;
  industry_cost[work_defense][resource_metal] = 0.2;
  industry_cost[work_defense][resource_gas] = 0.1;
  industry_cost[work_defense][resource_organics] = 0.1;

  // names
  work_names.resize(work_num);
  work_names[work_expansion] = "ind. expansion";
  work_names[work_research] = "research";
  work_names[work_resource] = "resource";
  work_names[work_ship] = "ship";
  work_names[work_defense] = "defense";

  sub_names.resize(work_num);

  // resource names
  sub_names[work_resource].resize(resource_num);
  sub_names[work_resource][resource_metal] = "metal";
  sub_names[work_resource][resource_gas] = "gas";
  sub_names[work_resource][resource_organics] = "organics";

  // ship names
  sub_names[work_ship].resize(ship_num);
  sub_names[work_ship][ship_scout] = "scout";
  sub_names[work_ship][ship_fighter] = "fighter";
  sub_names[work_ship][ship_bomber] = "bomber";
  sub_names[work_ship][ship_colonizer] = "colonizer";

  // research names
  sub_names[work_research].resize(research::r_num);
  sub_names[work_research][research::r_population] = "population";
  sub_names[work_research][research::r_industry] = "industry";
  sub_names[work_research][research::r_ship] = "ship";

  // defense names
  sub_names[work_defense].resize(defense_num);
  sub_names[work_defense][defense_enhance] = "enhance";
  sub_names[work_defense][defense_repair] = "repair";

  // work names
  sub_names[work_expansion] = work_names;

  // production per person
  p3.resize(work_num);
  p3[work_research] = 1e-3;
  p3[work_expansion] = 2e-3;
  p3[work_ship] = 2e-2;
  p3[work_resource] = 2e-2;
  p3[work_defense] = 2e-2;
}

void choice_t::normalize(){
  workers = fmin(fmax(workers, 0), 1);
  utility::normalize_vector(sector);
  for (auto &x : subsector) utility::normalize_vector(x);
}

choice_t::choice_t(){
  workers = 0.5;
  sector = vector<float>(work_num, 1);
  subsector.resize(work_num);
  subsector[work_expansion] = vector<float>(work_num, 1);
  subsector[work_ship] = vector<float>(ship_num, 1);
  subsector[work_research] = vector<float>(research::r_num, 1);
  subsector[work_resource] = vector<float>(resource_num, 1);
  subsector[work_defense] = vector<float>(defense_num, 1);
}

float st3::solar::solar::resource_constraint(std::vector<float> r){
  float max = INFINITY;

  for (int i = 0; i < resource_storage.size(); i++){
    max = fmin(max, resource_storage[i] / r[i]);
  }

  return max;
}

st3::solar::solar::solar(){
  population_number = 0;
  population_happy = 1;
  usable_area = 1e8 + utility::random_uniform() * 1e9;
  vision = 120;
  resource = utility::vscale(utility::random_uniform(resource_num), 1000);
  fleet_growth = vector<float>(ship_num, 0);
  new_research = vector<float>(research::r_num, 0);
  industry = vector<float>(work_num, 0);
  resource_storage = vector<float>(resource_num, 0);
  defense_current = defense_capacity = 0;
}

string st3::solar::solar::get_info(){
  stringstream ss;
  ss << "fleet_growth: " << fleet_growth << endl;
  ss << "new_research: " << new_research << endl;
  ss << "industry: " << industry << endl;
  ss << "resource storage: " << resource_storage << endl;
  ss << "pop: " << population_number << "(" << population_happy << ")" << endl;
  ss << "resource: " << resource << endl;
  ss << "ships: " << ships.size() << endl;
  ss << "defence: " << defense_current << "(" << defense_capacity << ")" << endl;
  return ss.str();
}

// increment per [sub]sector per unit time
float st3::solar::solar::sub_increment(research const &r, int sector_idx, int sub_idx, float workers){
  float rate;

  cout << "solar::sub_increment: " << sector_idx << "," << sub_idx << "," << workers << endl;

  switch(sector_idx){
  case work_research:
    rate = p3[work_research];
    break;
  case work_expansion:
    rate = p3[work_expansion] * r.field[research::r_industry];
    break;
  case work_ship:
    rate = p3[work_ship] * r.field[research::r_industry] / ship_buildtime[sub_idx];
    break;
  case work_resource:
    rate = p3[work_resource] * r.field[research::r_industry];
    break;
  case work_defense:
    if (sub_idx == defense_enhance){
      rate = 10 * p3[work_defense] * r.field[research::r_industry] / (defense_capacity + 1);
    }else if (sub_idx == defense_repair){
      rate = p3[work_defense] * r.field[research::r_industry];
    }
    break;
  }

  cout << "sub increment rate: " << (rate * workers * population_happy) << endl;

  return rate * workers * population_happy;
}

float st3::solar::solar::pop_increment(research const &r, float allocated){
  float i_sum = 0;
  for (auto x : industry) i_sum += x;
  float farmers = fmin(usable_area - i_sum, allocated);
  float food_cap = (food_per_person + fpp_per_research * utility::sigmoid(r.field[research::r_population], agriculture_limit_coefficient)) * farmers;
  float feed = feed_boost_coefficient * (food_cap - population_number) / population_number;
  float birth_rate = births_per_person + feed;
  float crowding = crowding_rate / (r.field[research::r_population] + 1) * population_number;
  float death_rate = deaths_per_person / (r.field[research::r_population] + 1) + fmax(-feed, 0) + crowding;

  cout << "pop: " << population_number << ", happy: " << population_happy << ", farmers: " << farmers << ", cap: " << food_cap << ", feed: " << feed << ", birth: " << birth_rate << ", crowding: " << crowding << ", death: " << death_rate << endl;

  return (birth_rate - death_rate) * population_number;
}
