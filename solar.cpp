#include <vector>
#include <iostream>

#include "research.h"
#include "solar.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace solar;

vector<vector<float> > development::ship_cost;
vector<float> development::ship_buildtime;
// vector<string> development::population_names;
vector<string> development::work_names;
vector<vector<string> > development::sub_names;
vector<float> development::p3;

const float st3::solar::births_per_person = 2e-3;
const float st3::solar::deaths_per_person = 2e-3;
const float st3::solar::agriculture_boost_coefficient = 1e3;
const float st3::solar::feed_boost_coefficient = 4e-3;

void development::initialize(){
  ship_cost.resize(ship_num);
  ship_buildtime.resize(ship_num);
  for (auto &x : ship_cost) x.resize(resource_num);

  // scout
  ship_cost[ship_scout][resource_metal] = 1;
  ship_cost[ship_scout][resource_gas] = 1;
  ship_buildtime[ship_scout] = 10;

  // fighter
  ship_cost[ship_fighter][resource_metal] = 1;
  ship_cost[ship_fighter][resource_gas] = 2;
  ship_buildtime[ship_fighter] = 20;

  // bomber
  ship_cost[ship_bomber][resource_metal] = 3;
  ship_cost[ship_bomber][resource_gas] = 2;
  ship_buildtime[ship_bomber] = 30;

  // industry names
  work_names.resize(work_num);
  work_names[work_expansion] = "ind. expansion";
  work_names[work_research] = "research";
  work_names[work_resource] = "resource";
  work_names[work_ship] = "ship";

  sub_names.resize(work_num);

  // resource names
  sub_names[work_resource].resize(resource_num);
  sub_names[work_resource][resource_metal] = "metal";
  sub_names[work_resource][resource_gas] = "gas";

  // ship names
  sub_names[work_ship].resize(ship_num);
  sub_names[work_ship][ship_scout] = "scout";
  sub_names[work_ship][ship_fighter] = "fighter";
  sub_names[work_ship][ship_bomber] = "bomber";

  // research names
  sub_names[work_research].resize(research::r_num);
  sub_names[work_research][research::r_population] = "population";
  sub_names[work_research][research::r_industry] = "industry";
  sub_names[work_research][research::r_ship] = "ship";

  // work names
  sub_names[work_expansion] = work_names;

  // production per person
  p3.resize(work_num);
  p3[work_research] = 1e-3;
  p3[work_expansion] = 2e-3;
  p3[work_ship] = 2e-2;
  p3[work_resource] = 2e-2;
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
}

float st3::solar::solar::resource_constraint(std::vector<float> r){
  float max = INFINITY;

  for (int i = 0; i < dev.resource.size(); i++){
    max = fmin(max, dev.resource[i] / r[i]);
  }

  return max;
}

st3::solar::solar::solar(){
  population_number = 1000;
  population_happy = 1;
  usable_area = 1e8 + utility::random_uniform() * 1e9;
  vision = 120;
  resource = vector<float>(resource_num, 1000);
  dev.fleet_growth = vector<float>(ship_num, 0);
  dev.new_research = vector<float>(research::r_num, 0);
  dev.industry = vector<float>(work_num, 200);
  dev.resource = vector<float>(resource_num, 0);
}

string st3::solar::solar::get_info(){
  stringstream ss;
  ss << "fleet_growth: " << dev.fleet_growth << endl;
  ss << "new_research: " << dev.new_research << endl;
  ss << "industry: " << dev.industry << endl;
  ss << "resource storage: " << dev.resource << endl;
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
    rate = development::p3[work_research];
    break;
  case work_expansion:
    rate = development::p3[work_expansion] * r.field[research::r_industry];
    break;
  case work_ship:
    rate = development::p3[work_ship] * r.field[research::r_industry] / development::ship_buildtime[sub_idx];
    break;
  case work_resource:
    rate = development::p3[work_resource] * r.field[research::r_industry];
    break;
  }

  return rate * workers * population_happy;
}

float st3::solar::solar::pop_increment(research const &r, float farmers){
  float food_cap = (1.5 + 0.01 * utility::sigmoid(r.field[research::r_population], agriculture_boost_coefficient)) * farmers;

  float feed = feed_boost_coefficient * (food_cap - population_number) / population_number;

  float birth_rate = births_per_person + feed;

  float death_rate = deaths_per_person / (r.field[research::r_population] + 1) + fmax(-feed, 0);

  cout << "pop: " << population_number << ", happy: " << population_happy << ", farmers: " << farmers << ", cap: " << food_cap << ", feed: " << feed << ", birth: " << birth_rate << ", death: " << death_rate << endl;

  return (birth_rate - death_rate) * population_number;
}
