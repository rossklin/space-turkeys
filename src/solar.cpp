#include <vector>
#include <iostream>
#include <rapidjson/document.h>

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
  research_level = &g -> players[owner].research_level;
  for (auto &t : development){
    if (t.second.is_turret){
      t.second.turret.load += 0.1;
    }
  }
}

// so far, solars don't move
void solar::move(game_data *g){
  if (owner < 0 || population <= 0) return;

  dt = g -> settings.dt;
  dynamics();

  // build ships
  research::data r = g -> players[owner].research_level;
  for (auto v : ship::all_classes()) {
    if (r.can_build_ship(v, ptr(this))){
      while (fleet_growth[v] >= 1) {
	fleet_growth[v]--;
	ship sh = r.build_ship(v, ptr(this));
	sh.is_landed = true;
	sh.owner = owner;
	ships.insert(sh.id);
	g -> add_entity(ship::ptr(new ship(sh)));
      }
    }
  }
}

list<combid> solar::confirm_interaction(string a, list<combid> t, game_data *g) {
  list<combid> result;
  if (!t.empty()) result.push_back(utility::uniform_sample(t));
  return result;
}

set<string> solar::compile_interactions(){
  return {interaction::turret_combat};
}

float solar::interaction_radius() {
  float r = 0;

  for (auto &t : development){
    if (t.second.is_turret){
      r = max(r, t.second.turret.range);
    }
  }
  
  return r;
}

void solar::receive_damage(game_object::ptr s, float damage, game_data *g){
  damage -= compute_shield_power();
  damage_facilities(damage);
  population *= 1 - 0.05 * damage;
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

  // repair facilities
  if (owner != game_object::neutral_owner) {
    for (auto &f : development) {
      if (f.second.hp < f.second.base_hp) f.second.hp += 0.01;
    }
  }
}

void solar::give_commands(list<command> c, game_data *g) {
  list<combid> buf;

  // create fleets
  for (auto &x : c){
    buf.clear();
    for (auto i : x.ships){
      if (!ships.count(i)) throw runtime_error("solar::give_commands: invalid ship id: " + i);
      buf.push_back(i);
      ships.erase(i);
    }

    x.origin = id;
    g -> generate_fleet(position, owner, x, buf);

    for (auto i : x.ships) g -> get_ship(i) -> on_liftoff(this, g);
  }
}

float solar::resource_constraint(cost::res_t r){
  float m = INFINITY;

  for (auto v : keywords::resource) {
    if (r[v] > 0) m = fmin(m, resource_storage[v] / r[v]);
  }

  return m;
}

void solar::pay_resources(cost::res_t total){
  for (auto k : keywords::resource) {
    resource_storage[k] = fmax(resource_storage[k] - total[k], 0);
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
  sfloat res = 5;
  for (auto &x : development) res = fmax(res, x.second.vision);
  return res + radius;
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
  if (owner == game_object::neutral_owner) return false;
  for (auto &t : development) if (t.second.is_turret) return true;
}

void solar::damage_facilities(float d){
  float d0 = d;
  while (d > 0 && development.size() > 0){
    list<string> remove;
    for (auto i = development.begin(); d > 0 && i != development.end(); i++){
      float k = fmin(utility::random_uniform(0, 0.1 * d0), d);
      i -> second.hp -= k;
      d -= k;
      if (i -> second.hp <= 0) remove.push_back(i -> first);
    }
    for (auto r : remove) development.erase(r);
  }
}

// [0,1] evaluation of available free space
float solar::space_status(){
  if (space <= 0) return 0;
  
  float used = 0;
  for (auto &t : development) used += cost::expansion_multiplier(t.second.level) * t.second.space_usage;

  if (used > space) throw runtime_error("space_status: used more than space");
  return (space - used) / space;
}

// [0,1] evaluation of available water
float solar::water_status(){
  if (water <= 0) return 0;
  
  float used = 0;
  for (auto &t : development) used += cost::expansion_multiplier(t.second.level) * t.second.water_usage;

  if (used > water) throw runtime_error("water status: used more than water!");
  return (water - used) / water;
}

float solar::population_increment(){
  float base_growth = population * happiness * f_growth;
  float culture_growth = base_growth * compute_boost(keywords::key_culture);
  float crowding_death = population * f_crowding * population / (ecology * space * space_status() + 1);
  return base_growth + culture_growth - crowding_death;
}

float solar::ecology_increment(){
  return 0.01 * ((space_status() * water_status() - ecology));
}

float solar::happiness_increment(choice::c_solar &c){
  return 0.01 * (0.2 * compute_boost(keywords::key_culture) + c.allocation[keywords::key_culture] - c.allocation[keywords::key_military] - 0.1 * log(population) / (ecology + 1) - (happiness - 0.5));
}

float solar::research_increment(choice::c_solar &c){
  return f_resrate * c.allocation[keywords::key_research] * population * compute_boost(keywords::key_research) * happiness;
}

float solar::resource_increment(string v, choice::c_solar &c){
  return f_minerate * compute_boost(keywords::key_mining) * c.allocation[keywords::key_mining] * c.mining[v] * compute_workers();
}

float solar::development_increment(choice::c_solar &c){
  return f_devrate * c.allocation[keywords::key_development] * compute_boost(keywords::key_development) * compute_workers();
}

float solar::ship_increment(string v, choice::c_solar &c){
  float build_time = ship::table().at(v).build_time;
  return f_buildrate * compute_boost(keywords::key_military) * c.allocation[keywords::key_military] * c.military[v] * compute_workers() / build_time;
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
    
    float random_growth = population * f_growth * utility::random_normal(0, 1);
    buf.population += population_increment() * dt + random_growth * dw;

    // happiness development
    buf.happiness += happiness_increment(c) * dt + utility::random_normal(0, 0.01) * dw;
    buf.happiness = fmax(fmin(1, buf.happiness), 0);

    // research and development
    buf.research_points += research_increment(c) * dt;
    buf.development_points += development_increment(c) * dt;

    // mining
    for (auto v : keywords::resource){
      float move = fmin(available_resource[v], dt * resource_increment(v, c));
      buf.available_resource[v] -= move;
      buf.resource_storage[v] += move;
    }

    // for all costs, sum desired quantity and cost
    float total_quantity = 0;
    cost::res_t total_cost;
    hm_t<string, float> weight_table;
    
    // military industry
    for (auto v : ship::all_classes()){
      float quantity = dt * ship_increment(v, c);
      cost::res_t build_cost = ship::table().at(v).build_cost;
      total_quantity += quantity;
      build_cost.scale(quantity);
      total_cost.add(build_cost);
      weight_table[v] = quantity;
    }

    float allowed = fmin(1, resource_constraint(total_cost));

    for (auto v : ship::all_classes()) {
      buf.fleet_growth[v] += allowed * weight_table[v];
    }

    total_cost.scale(allowed);
    buf.pay_resources(total_cost);
  }

  *this = buf;
}

bool solar::isa(string c) {
  return c == solar::class_id || c == physical_object::class_id || c == commandable_object::class_id;
}

bool solar::can_see(game_object::ptr x) {
  float r = 1;

  if (x -> isa(ship::class_id)) {
    ship::ptr s = utility::guaranteed_cast<ship>(x);
    r = vision() / (s -> stealth + 1);
  }
  
  float d = utility::l2norm(x -> position - position);
  return d < r;
}

float solar::compute_shield_power() {
  float sum = 0;
  for (auto &f : development) sum += f.second.shield;
  return log(sum + 1);
}

float solar::compute_boost(string v){
  float sum = 1;
  for (auto &f : development) {
    if (f.second.sector_boost.count(v)) {
      sum *= f.second.sector_boost[v] * f.second.level;
    }
  }

  auto &rtab = research::data::table();
  for (auto r : research_level -> researched) {
    research::tech t = rtab.at(r);
    if (t.sector_boost.count(v)) sum *= t.sector_boost[v];
  }
  
  return sum;
}

void solar::develop(string fac) {
  // initialize
  if (!development.count(fac)) {
    development[fac] = facility_table().at(fac);
  }

  // pay
  cost::res_t pay = development[fac].cost_resources;
  float multiplier = cost::expansion_multiplier(development[fac].level);
  pay.scale(multiplier);
  pay_resources(pay);
  development_points -= multiplier * development[fac].cost_time;

  // level up and repair
  development[fac].level++;
  development[fac].hp = development[fac].base_hp;
}

int solar::get_facility_level(string fac) {
  if (development.count(fac)) {
    return development[fac].level;
  }else{
    return 0;
  }
}

void facility::read_from_json(const rapidjson::Value &x) {
  development::node::read_from_json(x);
  
  if (x.HasMember("vision")) vision = x["vision"].GetDouble();
  if (x.HasMember("hp")) base_hp = x["hp"].GetDouble();
  if (x.HasMember("shield")) shield = x["shield"].GetDouble();

  if (x.HasMember("turret")){
    is_turret = 1;
    auto &t = x["turret"];
    if (t.HasMember("damage")) turret.damage = t["damage"].GetDouble();
    if (t.HasMember("range")) turret.range = t["range"].GetDouble();
    if (t.HasMember("accuracy")) turret.accuracy = t["accuracy"].GetDouble();
    if (t.HasMember("load")) turret.load = t["load"].GetDouble();
  }

  if (x.HasMember("cost")) {
    auto &c = x["cost"];
    for (auto k : keywords::resource) {
      if (c.HasMember(k.c_str())) cost_resources[k] = c[k.c_str()].GetDouble();
    }
  }
  
  if (x.HasMember("water usage")) water_usage = x["water usage"].GetDouble();
  if (x.HasMember("space usage")) space_usage = x["space usage"].GetDouble();
}

const hm_t<string, facility>& solar::facility_table(){
  static hm_t<string, facility> data;
  static bool init = false;

  if (init) return data;
  
  rapidjson::Document *docp = utility::get_json("solar");
  facility f_init;
  // TODO: set basic facility stats
  data = development::read_from_json<facility>((*docp)["solar"], f_init);
  delete docp;
  init = true;
  return data;
}
    
facility_object::facility_object() : facility() {
  level = 0;
  hp = 0;
}
    
facility_object::facility_object(const facility &f) : facility(f) {
  level = 0;
  hp = 0;
}

facility::facility() : development::node() {
  vision = 0;
  is_turret = 0;
  base_hp = 0;
  shield = 0;
}

facility::facility(const facility &f) : development::node(f) {
  *this = f;
}

turret_t::turret_t(){
  range = 0;
  damage = 0;
  accuracy = 0;
  load = 0;
}

list<string> solar::available_facilities(const research::data &r_level) {
  list<string> r;
  auto &t = facility_table();

  for (auto &x : t){
    facility f = x.second;
    bool pass = true;
    float level_multiplier = cost::expansion_multiplier(get_facility_level(x.first));

    // check cost
    cost::res_t res_buf = f.cost_resources;
    res_buf.scale(level_multiplier);
    pass &= resource_constraint(res_buf) >= 1;
    pass &= development_points >= level_multiplier * f.cost_time;
    pass &= water * water_status() >= level_multiplier * f.water_usage;
    pass &= space * space_status() >= level_multiplier * f.space_usage;

    // check dependencies
    for (auto &y : f.depends_facilities) pass &= get_facility_level(y.first) >= y.second;
    for (auto &y : f.depends_techs) pass &= r_level.researched.count(y);
    
    if (pass) r.push_back(x.first);
  }

  return r;
}
