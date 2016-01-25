#include <vector>
#include <iostream>

#include "research.h"
#include "solar.h"
#include "utility.h"
#include "cost.h"

using namespace std;
using namespace st3;
using namespace solar;

float solar::solar::resource_constraint(cost::resource_allocation<sfloat> r){
  float m = INFINITY;

  for (auto v : cost::keywords::resource)
    if (r[v] > 0) m = fmin(m, resource[v].storage / r[v]);

  return m;
}

void solar::solar::pay_resources(cost::resource_allocation<float> total){
  for (auto k : cost::keywords::resource)
    resource[k].storage -= total[k];
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
  for (auto v : cost::keywords::sector)
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
  for (auto v : cost::keywords::sector)
    used += sector[v] * cost::sector_expansion()[v].water;

  return (water - used) / water;
}
