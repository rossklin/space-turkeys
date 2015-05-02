#include <vector>

#include "research.h"
#include "solar.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace solar;

vector<vector<float> > development::ship_cost;
vector<float> development::ship_buildtime;

const float st3::solar::research_per_person = 0.0001;
const float st3::solar::industry_per_person = 0.002;
const float st3::solar::fleet_per_person = 0.01;
const float st3::solar::resource_per_person = 0.001;
const float st3::solar::births_per_person = 0.0002;
const float st3::solar::deaths_per_person = 0.0001;
const float st3::solar::industry_growth_cap = 1;

void development::initialize(){
  ship_cost.resize(s_num);
  ship_buildtime.resize(s_num);
  for (auto &x : ship_cost) x.resize(o_num);

  // scout
  ship_cost[s_scout][o_metal] = 1;
  ship_cost[s_scout][o_gas] = 1;
  ship_buildtime[s_scout] = 10;

  // fighter
  ship_cost[s_fighter][o_metal] = 1;
  ship_cost[s_fighter][o_gas] = 2;
  ship_buildtime[s_fighter] = 20;

  // bomber
  ship_cost[s_bomber][o_metal] = 3;
  ship_cost[s_bomber][o_gas] = 2;
  ship_buildtime[s_bomber] = 30;
}

void choice_t::normalize(){
  utility::normalize_vector(population);
  utility::normalize_vector(dev.fleet_growth);
  utility::normalize_vector(dev.new_research);
  utility::normalize_vector(dev.industry);
  utility::normalize_vector(dev.resource);
}

choice_t::choice_t(){
  population = vector<float>(p_num, 1);
  dev.fleet_growth = vector<float>(s_num, 1);
  dev.new_research = vector<float>(research::r_num, 1);
  dev.industry = vector<float>(i_num, 1);
  dev.resource = vector<float>(o_num, 1);
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
  resource = vector<float>(o_num, 1000);
  dev.fleet_growth = vector<float>(s_num, 0);
  dev.new_research = vector<float>(research::r_num, 0);
  dev.industry = vector<float>(i_num, 200);
  dev.resource = vector<float>(o_num, 0);
}
