#include <vector>

#include "research.h"
#include "solar.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace solar;

vector<vector<float> > development::ship_cost({{1,2,3}, {1,2,3}, {1,2,3}});

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

float solar::solar::resource_constraint(std::vector<float> r){
  return 100;
}

solar::solar::solar(){
  population_number = 1000;
  population_happy = 1;
  resource = vector<float>(o_num, 100);
  dev.fleet_growth = vector<float>(s_num, 0);
  dev.new_research = vector<float>(research::r_num, 0);
  dev.industry = vector<float>(i_num, 1);
  dev.resource = vector<float>(o_num, 1000);
}
