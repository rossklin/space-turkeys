#include <vector>

#include "research.h"
#include "solar.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace solar;

vector<vector<float> > development::ship_cost;
vector<float> development::ship_buildtime;
vector<string> development::population_names;
vector<string> development::industry_names;
vector<string> development::research_names;
vector<string> development::resource_names;
vector<string> development::ship_names;

const float st3::solar::research_per_person = 1e-3;
const float st3::solar::industry_per_person = 2e-3;
const float st3::solar::fleet_per_person = 1e-3;
const float st3::solar::resource_per_person = 1e-3;
const float st3::solar::births_per_person = 2e-3;
const float st3::solar::deaths_per_person = 2e-3;
const float st3::solar::agriculture_boost_coefficient = 1e3;
const float st3::solar::feed_boost_coefficient = 4e-3;

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

  // population field names
  population_names.resize(p_num);
  population_names[p_research] = "research";
  population_names[p_industry] = "industry";
  population_names[p_resource] = "resource";
  population_names[p_agriculture] = "agriculture";

  // industry names
  industry_names.resize(i_num);
  industry_names[i_infrastructure] = "infrastructure";
  industry_names[i_agriculture] = "agriculture";
  industry_names[i_research] = "research";
  industry_names[i_resource] = "resource";
  industry_names[i_ship] = "ship";

  // resource names
  resource_names.resize(o_num);
  resource_names[o_metal] = "metal";
  resource_names[o_gas] = "gas";

  // ship names
  ship_names.resize(s_num);
  ship_names[s_scout] = "scout";
  ship_names[s_fighter] = "fighter";
  ship_names[s_bomber] = "bomber";

  // research names
  research_names.resize(research::r_num);
  research_names[research::r_population] = "population";
  research_names[research::r_industry] = "industry";
  research_names[research::r_ship] = "ship";
  
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
  usable_area = 1e8 + utility::random_uniform() * 1e9;
  resource = vector<float>(o_num, 1000);
  dev.fleet_growth = vector<float>(s_num, 0);
  dev.new_research = vector<float>(research::r_num, 0);
  dev.industry = vector<float>(i_num, 200);
  dev.resource = vector<float>(o_num, 0);
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
