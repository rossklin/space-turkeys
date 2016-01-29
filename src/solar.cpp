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

solar::solar solar::solar::dynamics(choice::c_solar &c, float dt){
  c.normalize();
  float workers = happiness * population;
  auto buf = *this;
  float base_growth = population * happiness * st3::solar::f_growth;
  float culture_growth = base_growth * sector[cost::keywords::key_culture];
  float random_growth = population * st3::solar::f_growth * utility::random_normal(0, 1);
  float crowding_death = population * st3::solar::f_crowding * population / (ecology * space * space_status() + 1);
  float pop_growth = base_growth + culture_growth + random_growth - crowding_death;

  // debug printout
  // cout << "dynamics: " << population << ": happy: " << happiness << ", growth: " << base_growth << ", cgrowth: " << culture_growth << ", cdeath: " << crowding_death << ", random: " << random_growth << ", total: " << pop_growth;

  // ecological development
  buf.ecology += 0.01 * ((space_status() * water_status() - ecology) + utility::random_normal(0, 1)) * dt;
  buf.ecology = fmax(fmin(buf.ecology, 1), 0);

  if (population > 0){
    // population development
    
    buf.population += pop_growth * dt;

    // happiness development
    buf.happiness += 0.01 * (sector[cost::keywords::key_culture] + c.allocation[cost::keywords::key_culture] - 0.1 * log(population) / (ecology + 1) + utility::random_normal(0,1) - (happiness - 0.5)) * dt;
    buf.happiness = fmax(fmin(1, buf.happiness), 0);

    // research development
    buf.research += c.allocation[cost::keywords::key_research] * buf.population * buf.sector[cost::keywords::key_research] * buf.happiness * dt;

    // expansions
    for (auto v : cost::keywords::expansion){
      float level = floor(sector[v]);
      float multiplier = pow(2, level);
      cost::resource_allocation<float> effective_cost = multiplier * cost::sector_expansion()[v].res;

      float quantity = fmin(dt * st3::solar::f_buildrate * c.allocation[cost::keywords::key_expansion] * c.expansion[v] * workers / (multiplier * cost::sector_expansion()[v].time), resource_constraint(effective_cost));

      if (quantity < 0){
	cout << "error" << endl;
      }
      
      buf.sector[v] += quantity;
      buf.pay_resources(quantity * effective_cost);
    }

    // mining
    for (auto v : cost::keywords::resource){
      float move = fmin(resource[v].available, dt * st3::solar::f_minerate * c.allocation[cost::keywords::key_mining] * c.mining[v] * workers);
      buf.resource[v].available -= move;
      buf.resource[v].storage += move;
    }

    // military industry
    for (auto v : cost::keywords::ship){
      auto build_cost = cost::ship_build()[v];
      float quantity = fmin(dt * st3::solar::f_buildrate * c.allocation[cost::keywords::key_military] * c.military.c_ship[v] * workers / build_cost.time, resource_constraint(build_cost.res));
      buf.fleet_growth[v] += quantity;
      buf.pay_resources(quantity * build_cost.res);
    }
  
    for (auto v : cost::keywords::turret){
      auto build_cost = cost::turret_build()[v];
      float quantity = fmin(dt * st3::solar::f_buildrate * c.allocation[cost::keywords::key_military] * c.military.c_turret[v] * workers / build_cost.time, resource_constraint(build_cost.res));
      buf.turret_growth[v] += quantity;
      buf.pay_resources(quantity * build_cost.res);
    }
  }

  return buf;
}
