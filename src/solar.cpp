#include <vector>
#include <iostream>

#include "research.h"
#include "solar.h"
#include "utility.h"
#include "cost.h"
#include "choice.h"
#include "game_data.h"
#include "interaction.h"

using namespace std;
using namespace st3;

void solar::pre_phase(game_data *g){
  for (auto &t : turrets) t.load = fmin(t.load + 1, t.load_time);
  damage_taken.clear();
  colonization_attempts.clear();
}

// so far, solars don't move
void solar::move(game_data *g){
  if (owner < 0 || population <= 0) return;
  
  dynamics();

  // todo: add ship and turret build
}

void solar::interact(game_data *g){
  
  for (auto &t : turrets){
    if (t.damage > 0 && t.load >= t.load_time){

      // find targetable ships
      list<combid> buf = g -> search_targets(position, t.range, target_condition(owner, target_condition::enemy, identifier::ship));
      
      // fire at a random enemy
      if (!buf.empty()){
	t.load = 0;
	
	combid tid = utility::uniform_sample(buf);
	ship::ptr s = g -> get_ship(tid);
	
	if (utility::random_uniform() < t.accuracy){
	  s -> receive_damage(ptr(this), s, utility::random_uniform(0, t.damage));
	}
      }
    }
  }
}

void solar::post_phase(game_data *g){
  float total_damage = 0;
  float highest_id = -1;
  float highest_sum = 0;

  // analyse damage
  for (auto x : damage_taken) {
    total_damage += x.second;
    if (x.second > highest_sum){
      highest_sum = x.second;
      highest_id = x.first;
    }
    cout << "solar_effects: " << x.second << " damage from player " << x.first << endl;
  }

  if (highest_id > -1){
    // todo: add some random destruction to solar
    damage_turrets(total_damage);
    population = fmax(population - 10 * total_damage, 0);
    happiness *= 0.9;

    if (owner > -1 && !has_defense()){
      owner = highest_id;
      cout << "player " << owner << " conquers " << id << endl;
    }else{
      cout << "resulting defense for " << id << ": " << has_defense() << endl;
    }
  }

  // colonization: randomize among attempts
  float count = 0;
  float num = colonization_attempts.size();
  for (auto i : colonization_attempts){
    if (utility::random_uniform() <= 1 / (num - count++)){
      g -> players[i.first].research_level.colonize(ptr(this));
      owner = i.first;
      cout << "player " << owner << " colonizes " << id << endl;
      g -> entity[i.second] -> remove = true;
    }
  }
}

void solar::give_commands(list<command> c, game_data *g){
  list<ship> buf;

  // create fleets
  for (auto &x : c){
    buf.clear();
    for (auto i : x.ships){
      if (!ships.count(i)){
	cout << "solar::give_commands: invalid ship id: " << i << endl;
	exit(-1);
      }
      buf.push_back(ships[i]);
      ships.erase(i);
    }

    g -> generate_fleet(position, owner, x, buf);
  }
}

float solar::resource_constraint(cost::resource_allocation<sfloat> r){
  float m = INFINITY;

  for (auto v : cost::keywords::resource)
    if (r[v] > 0) m = fmin(m, resource[v].storage / r[v]);

  return m;
}

void solar::pay_resources(cost::resource_allocation<float> total){
  for (auto k : cost::keywords::resource)
    resource[k].storage = fmax(resource[k].storage - total[k], 0);
}

solar::solar(){}

string solar::get_info(){
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

sfloat solar::vision(){
  sfloat res = 0;

  for (auto &x : turrets)
    res = fmax(res, x.vision);

  return res;
}

solar::ptr solar::create(){
  return ptr(new solar());
}

solar::ptr solar::clone(){
  return dynamic_pointer_cast<solar>(clone_impl());
}

game_object::ptr solar::clone_impl(){
  return ptr(new solar(*this));
}

bool solar::has_defense(){
  return turrets.size() > 0;
}

void solar::damage_turrets(float d){
  float d0 = d;
  if (turrets.empty()) return;

  while (d > 0 && turrets.size() > 0){
    for (auto i = turrets.begin(); d > 0 && i != turrets.end();){
      float k = fmin(utility::random_uniform(0, 0.1 * d0), d);
      i -> hp -= k;
      d -= k;
      if (i -> hp <= 0) {
	turrets.erase(i++);
      }else{
	i++;
      }
    }
  }
}

// [0,1] evaluation of available free space
float solar::space_status(){
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
float solar::water_status(){
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

float solar::poluation_increment(){
  float base_growth = population * happiness * st3::solar::f_growth;
  float culture_growth = base_growth * sector[cost::keywords::key_culture];
  float crowding_death = population * st3::solar::f_crowding * population / (ecology * space * space_status() + 1);
  return base_growth + culture_growth - crowding_death;
}

float solar::ecology_increment(){
  return 0.01 * ((space_status() * water_status() - ecology));
}

float solar::happiness_increment(choice::c_solar &c){
  return 0.01 * (sector[cost::keywords::key_culture] + c.allocation[cost::keywords::key_culture] - 0.1 * log(population) / (ecology + 1) - (happiness - 0.5));
}

float solar::research_increment(choice::c_solar &c){
  return c.allocation[cost::keywords::key_research] * population * sector[cost::keywords::key_research] * happiness;
}

float solar::resource_increment(string v, choice::c_solar &c){
  return st3::solar::f_minerate * c.allocation[cost::keywords::key_mining] * c.mining[v] * compute_workers();
}

float solar::expansion_increment(string v, choice::c_solar &c){
  return st3::solar::f_buildrate * c.allocation[cost::keywords::key_expansion] * c.expansion[v] * compute_workers() / (cost::expansion_multiplier(sector[v]) * cost::sector_expansion()[v].time);
}

float solar::ship_increment(string v, choice::c_solar &c){
  auto build_cost = cost::ship_build()[v];
  return st3::solar::f_buildrate * c.allocation[cost::keywords::key_military] * c.military.c_ship[v] * compute_workers() / build_cost.time;
}

float solar::turret_increment(string v, choice::c_solar &c){
  auto build_cost = cost::turret_build()[v];
  return st3::solar::f_buildrate * c.allocation[cost::keywords::key_military] * c.military.c_turret[v] * compute_workers() / build_cost.time;
}

float solar::compute_workers(){
  return happiness * population;
}

void solar::dynamics(){
  choice::c_solar c = choice_data;
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

  *this = buf;
}
