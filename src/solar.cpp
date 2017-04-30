#include <vector>
#include <iostream>

#include "research.h"
#include "solar.h"
#include "utility.h"
#include "cost.h"
#include "choice.h"
#include "game_data.h"
#include "interaction.h"
#include "serialization.h"

using namespace std;
using namespace st3;

const string solar::class_id = "solar";

solar::solar(){}

solar::~solar(){}

void solar::pre_phase(game_data *g){
  for (auto &t : turrets) t.load = fmin(t.load + 1, t.load_time);
  damage_taken.clear();
  colonization_attempts.clear();
}

// so far, solars don't move
void solar::move(game_data *g){
  if (owner < 0 || population <= 0) return;

  dt = g -> settings.dt;
  dynamics();

  // build ships and turrets
  research::data r = g -> players[owner].research_level;
  for (auto v : cost::keywords::ship) {
    while (fleet_growth[v] >= 1) {
      fleet_growth[v]--;
      ship sh = r.build_ship(v);
      sh.owner = owner;
      ships.insert(sh.id);
      g -> add_entity(ship::ptr(new ship(sh)));
    }
  }

  for (auto v : cost::keywords::turret) {
    while (turret_growth[v] >= 1) {
      turret_growth[v]--;
      turrets.push_back(r.build_turret(v));
    }
  }
}

bool solar::confirm_interaction(string a, combid t, game_data *g) {
  return true;
}

set<string> solar::compile_interactions(){
  return {fleet_action::turret_combat};
}

float solar::interaction_radius() {
  float r = 0;
  for (auto &t : turrets) r = max(r, t.range);
  return r;
}

void solar::receive_damage(game_object::ptr s, float damage, game_data *g){
  damage_turrets(damage);
  population = fmax(population - 10 * damage, 0);
  happiness *= 0.9;

  if (owner != game_object::neutral_owner && !has_defense()){
    owner = s -> owner;
    cout << "player " << owner << " conquers " << id << endl;

    // switch owners for ships on solar
    for (auto sid : ships) g -> get_ship(sid) -> owner = owner;
  }else{
    cout << "resulting defense for " << id << ": " << has_defense() << endl;
  }
}

void solar::post_phase(game_data *g){
  if (owner != game_object::neutral_owner && population == 0){
    // everyone here is dead, this is now a neutral solar
    owner = game_object::neutral_owner;
  }
}

void solar::give_commands(list<command> c, game_data *g){
  list<combid> buf;

  // create fleets
  for (auto &x : c){
    buf.clear();
    for (auto i : x.ships){
      if (!ships.count(i)) throw runtime_error("solar::give_commands: invalid ship id: " + i);
      buf.push_back(i);
      ships.erase(i);
    }

    g -> generate_fleet(position, owner, x, buf);
  }
}

float solar::resource_constraint(cost::resource_allocation<sfloat> r){
  float m = INFINITY;

  for (auto v : cost::keywords::resource) {
    if (r[v] > 0) m = fmin(m, resource[v].storage / r[v]);
  }

  return m;
}

void solar::pay_resources(cost::resource_allocation<float> total){
  for (auto k : cost::keywords::resource) {
    resource[k].storage = fmax(resource[k].storage - total[k], 0);
  }
}

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
  for (auto &x : turrets) res = fmax(res, x.vision);
  return res;
}

solar::ptr solar::create(){
  return ptr(new solar());
}

solar::ptr solar::clone(){
  return dynamic_cast<solar::ptr>(clone_impl());
}

game_object::ptr solar::clone_impl(){
  return ptr(new solar(*this));
}

void solar::copy_from(const solar &s){
  (*this) = s;
}

bool solar::serialize(sf::Packet &p){
  return p << class_id << *this;
}

bool solar::has_defense(){
  return owner != game_object::neutral_owner && turrets.size() > 0;
}

void solar::damage_turrets(float d){
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
float solar::space_status(){
  if (space <= 0) return 0;
  
  float used = 0;
  for (auto v : cost::keywords::expansion) {
    used += sector[v] * cost::sector_expansion()[v].space;
  }

  if (used > space) throw runtime_error("space_status: used more than space");
  return (space - used) / space;
}

// [0,1] evaluation of available water
float solar::water_status(){
  if (water <= 0) return 0;
  
  float used = 0;
  for (auto v : cost::keywords::expansion) {
    used += sector[v] * cost::sector_expansion()[v].water;
  }

  if (used > water) throw runtime_error("water status: used more than water!");
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
  return 0.01 * (0.2 * sector[cost::keywords::key_culture] + c.allocation[cost::keywords::key_culture] - c.allocation[cost::keywords::key_military] - 0.1 * log(population) / (ecology + 1) - (happiness - 0.5));
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
  solar buf = *this;
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

    for (auto v : cost::keywords::ship) {
      buf.fleet_growth[v] += allowed * weight_table[v];
    }

    for (auto v : cost::keywords::turret) {
      buf.turret_growth[v] += allowed * weight_table[v];
    }

    for (auto v : cost::keywords::expansion) {
      buf.sector[v] += allowed * weight_table[v];
    }

    total_cost.scale(allowed);
    buf.pay_resources(total_cost);
  }

  *this = buf;
}

bool solar::isa(string c) {
  return c == solar::class_id || c == physical_object::class_id || c == commandable_object::class_id;
}
