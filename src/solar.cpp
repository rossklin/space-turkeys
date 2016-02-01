#include <vector>
#include <iostream>

#include "research.h"
#include "solar.h"
#include "utility.h"
#include "cost.h"
#include "choice.h"

using namespace std;
using namespace st3;
using namespace solar;

const float solar::f_growth = 4e-2;
const float solar::f_crowding = 2e-2;
const float solar::f_minerate = 1e-2;
const float solar::f_buildrate = 1e-1;

float solar::solar::resource_constraint(cost::resource_allocation<sfloat> r){
  float m = INFINITY;

  for (auto v : cost::keywords::resource)
    if (r[v] > 0) m = fmin(m, resource[v].storage / r[v]);

  return m;
}

void solar::solar::pay_resources(cost::resource_allocation<float> total){
  for (auto k : cost::keywords::resource)
    resource[k].storage = fmax(resource[k].storage - total[k], 0);
}

solar::solar::solar(){}

string solar::solar::get_info(){
  stringstream ss;
  // ss << "fleet_growth: " << fleet_growth << endl;
  // ss << "new_research: " << new_research << endl;
  // ss << "industry: " << industry << endl;
  // ss << "resource storage: " << resource_storage << endl;
  ss << "pop: " << population << "(" << happiness << ")" << endl;
  // ss << "resource: " << resource << endl;
  ss << "ships: " << ships.size() << endl;
  return ss.str();
}

sfloat solar::solar::compute_vision(){
  sfloat res = 0;

  for (auto &x : turrets)
    res = fmax(res, x.vision);

  return res;
}

bool solar::solar::has_defense(){
  return turrets.size() > 0;
}

void solar::solar::damage_turrets(float d){
  float d0 = d;
  if (turrets.empty()) return;

  while (d > 0 && turrets.size() > 0){
    for (auto i = turrets.begin(); d > 0 && i != turrets.end(); i++){
      float k = fmin(utility::random_uniform(0, 0.1 * d0), d);
      i -> hp -= k;
      d -= k;
      if (i -> hp <= 0) turrets.erase(i--);
    }
  }
}

// [0,1] evaluation of available free space
float solar::solar::space_status(){
  if (space <= 0) return 0;
  
  float used = 0;
  for (auto v : cost::keywords::expansion)
    used += sector[v] * cost::sector_expansion()[v].space;

  if (used > space){
    cout << "space_status: used more than space" << endl;
    exit(-1);
  }

  return (space - used) / space;
}

// [0,1] evaluation of available water
float solar::solar::water_status(){
  if (water <= 0) return 0;
  
  float used = 0;
  for (auto v : cost::keywords::expansion)
    used += sector[v] * cost::sector_expansion()[v].water;

  if (used > water){
    cout << "water status: used more than water!" << endl;
    exit(-1);
  }

  return (water - used) / water;
}

float solar::solar::poluation_increment(){
  float base_growth = population * happiness * st3::solar::f_growth;
  float culture_growth = base_growth * sector[cost::keywords::key_culture];
  float crowding_death = population * st3::solar::f_crowding * population / (ecology * space * space_status() + 1);
  return base_growth + culture_growth - crowding_death;
}

float solar::solar::ecology_increment(){
  return 0.01 * ((space_status() * water_status() - ecology));
}

float solar::solar::happiness_increment(choice::c_solar &c){
  return 0.01 * (sector[cost::keywords::key_culture] + c.allocation[cost::keywords::key_culture] - 0.1 * log(population) / (ecology + 1) - (happiness - 0.5));
}

float solar::solar::research_increment(choice::c_solar &c){
  return c.allocation[cost::keywords::key_research] * population * sector[cost::keywords::key_research] * happiness;
}

float solar::solar::resource_increment(string v, choice::c_solar &c){
  return st3::solar::f_minerate * c.allocation[cost::keywords::key_mining] * c.mining[v] * compute_workers();
}

float solar::solar::expansion_increment(string v, choice::c_solar &c){
  return st3::solar::f_buildrate * c.allocation[cost::keywords::key_expansion] * c.expansion[v] * compute_workers() / (cost::expansion_multiplier(sector[v]) * cost::sector_expansion()[v].time);
}

float solar::solar::ship_increment(string v, choice::c_solar &c){
  auto build_cost = cost::ship_build()[v];
  return st3::solar::f_buildrate * c.allocation[cost::keywords::key_military] * c.military.c_ship[v] * compute_workers() / build_cost.time;
}

float solar::solar::turret_increment(string v, choice::c_solar &c){
  auto build_cost = cost::turret_build()[v];
  return st3::solar::f_buildrate * c.allocation[cost::keywords::key_military] * c.military.c_turret[v] * compute_workers() / build_cost.time;
}

float solar::solar::compute_workers(){
  return happiness * population;
}

solar::solar solar::solar::dynamics(choice::c_solar &c, float dt){
  c.normalize();
  auto buf = *this;
  float dw = sqrt(dt);

  // ecological development
  buf.ecology += ecology_increment() * dt + utility::random_normal(0, 0.01) * dw;
  buf.ecology = fmax(fmin(buf.ecology, 1), 0);

  if (population > 0){
    // population development
    
    float random_growth = population * st3::solar::f_growth * utility::random_normal(0, 1);
    buf.population += poluation_increment() * dt + random_growth * dw;

    // happiness development
    buf.happiness += happiness_increment(c) * dt + utility::random_normal(0, 0.01) * dw;
    buf.happiness = fmax(fmin(1, buf.happiness), 0);

    // research development
    buf.research += research_increment(c) * dt;

    // mining
    for (auto v : cost::keywords::resource){
      float move = fmin(resource[v].available, dt * resource_increment(v, c));
      buf.resource[v].available -= move;
      buf.resource[v].storage += move;
    }

    // for all costs, sum desired quantity and cost
    float total_quantity = 0;
    cost::countable_resource_allocation<float> total_cost;
    hm_t<string, float> weight_table;

    // expansions
    for (auto v : cost::keywords::expansion){
      cost::countable_resource_allocation<float> effective_cost = cost::sector_expansion()[v].res;
      effective_cost.scale(cost::expansion_multiplier(sector[v]));
      float quantity = dt * expansion_increment(v, c);

      total_quantity += quantity;
      effective_cost.scale(quantity);
      total_cost.add(effective_cost);
      weight_table[v] = quantity;
    }
    
    // military industry
    for (auto v : cost::keywords::ship){
      float quantity = dt * ship_increment(v, c);
      auto build_cost = cost::ship_build()[v];
      total_quantity += quantity;
      build_cost.res.scale(quantity);
      total_cost.add(build_cost.res);
      weight_table[v] = quantity;
    }

    for (auto v : cost::keywords::turret){
      float quantity = dt * turret_increment(v, c);
      auto build_cost = cost::turret_build()[v];
      total_quantity += quantity;
      build_cost.res.scale(quantity);
      total_cost.add(build_cost.res);
      weight_table[v] = quantity;
    }

    float allowed = fmin(1, resource_constraint(total_cost));

    for (auto v : cost::keywords::ship)
      buf.fleet_growth[v] += allowed * weight_table[v];

    for (auto v : cost::keywords::turret)
      buf.turret_growth[v] += allowed * weight_table[v];

    for (auto v : cost::keywords::expansion)
      buf.sector[v] += allowed * weight_table[v];

    total_cost.scale(allowed);
    buf.pay_resources(total_cost);
  }

  return buf;
}
