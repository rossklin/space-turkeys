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
  for (auto &t : development.facilities){
    if (t.second.is_turret){
      t.second.turret.load++;
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
  for (auto v : keywords::ship) {
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
  result.push_back(utility::uniform_sample(t));
  return result;
}

set<string> solar::compile_interactions(){
  return {interaction::turret_combat};
}

float solar::interaction_radius() {
  float r = 0;

  for (auto &t : development.facilities){
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
  for (auto &x : development.facilities) res = fmax(res, x.second.vision);
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
  for (auto &t : development.facilities) if (t.second.is_turret) return true;
}

void solar::damage_facilities(float d){
  float d0 = d;
  if (development.facilities.empty()) return;

  while (d > 0 && development.facilities.size() > 0){
    list<string> remove;
    for (auto i = development.facilities.begin(); d > 0 && i != development.facilities.end(); i++){
      float k = fmin(utility::random_uniform(0, 0.1 * d0), d);
      i -> second.hp -= k;
      d -= k;
      if (i -> second.hp <= 0) remove.push_back(i -> first);
    }
    for (auto r : remove) development.facilities.erase(r);
  }
}

// [0,1] evaluation of available free space
float solar::space_status(){
  if (space <= 0) return 0;
  
  float used = 0;
  for (auto &t : development.facilities) used += t.second.cost.space;

  if (used > space) throw runtime_error("space_status: used more than space");
  return (space - used) / space;
}

// [0,1] evaluation of available water
float solar::water_status(){
  if (water <= 0) return 0;
  
  float used = 0;
  for (auto &t : development.facilities) used += t.second.cost.water;

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
  return c.allocation[keywords::key_research] * population * compute_boost(keywords::key_research) * happiness;
}

float solar::resource_increment(string v, choice::c_solar &c){
  return f_minerate * (1 + compute_boost(keywords::key_mining)) * c.allocation[keywords::key_mining] * c.mining[v] * compute_workers();
}

float solar::development_increment(choice::c_solar &c){
  return f_buildrate * c.allocation[keywords::key_development] * compute_workers();
}

float solar::ship_increment(string v, choice::c_solar &c){
  auto build_cost = cost::ship_build()[v];
  return f_buildrate * compute_boost(keywords::key_military) * c.allocation[keywords::key_military] * c.military[v] * compute_workers() / build_cost.time;
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
    buf.development_points += research_increment(c) * dt;

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
    for (auto v : keywords::ship){
      float quantity = dt * ship_increment(v, c);
      auto build_cost = cost::ship_build()[v];
      total_quantity += quantity;
      build_cost.res.scale(quantity);
      total_cost.add(build_cost.res);
      weight_table[v] = quantity;
    }

    float allowed = fmin(1, resource_constraint(total_cost));

    for (auto v : keywords::ship) {
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

const hm_t<string, facility>& development_tree::table(){
  static hm_t<string, facility> data;
  static bool init = false;

  if (init) return data;
  rapidjson::Document doc = utility::get_json("solar");

  facility base, a;
  
  for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); i++){
    a = base;
    a.name = i -> name.GetString();

    if (i -> value.HasMember("sector boost")) {
      for (auto &b : i -> value["sector boost"]) {
	a.sector_boost[b.name.GetString()] = b.value.GetFloat();
      }
    }

    if (i -> value.HasMember("vision")) a.vision = i -> value["vision"].GetFloat();
    if (i -> value.HasMember("hp")) a.hp = i -> value["hp"].GetFloat();
    if (i -> value.HasMember("shield")) a.shield = i -> value["shield"].GetFloat();

    if (i -> value.HasMember("turret")){
      a.is_turret = 1;
      auto t = i -> value["turret"];
      if (t.HasMember("damage")) a.turret.damage = t["damage"].GetFloat();
      if (t.HasMember("range")) a.turret.range = t["range"].GetFloat();
      if (t.HasMember("accuracy")) a.turret.accuracy = t["accuracy"].GetFloat();
      if (t.HasMember("load")) a.turret.load = t["load"].GetFloat();
    }

    if (i -> value.HasMember("ship upgrades")) {
      for (auto &u : i -> value["ship upgrades"]) {
	string ship_name = u.name.GetString();
	for (auto &v : u.value) a.ship_upgrades[ship_name].insert(v.GetString());
      }
    }

    if (i -> value.HasMember("cost")) {
      auto c = i -> value["cost"];
      if (c.HasMember("resource")) {
	auto r = c["resource"];
	for (auto k : keywords::resource) {
	  if (r.HasMember(k)) a.cost.res[k] = r[k].GetFloat();
	}
      }
      if (c.HasMember("water")) a.cost.water = c["water"].GetFloat();
      if (c.HasMember("space")) a.cost.space = c["space"].GetFloat();
      if (c.HasMember("time")) a.cost.time = c["time"].GetFloat();
    }

    if (i -> value.HasMember("depends facilities")) {
      for (auto &d : i -> value["depends facilities"]) {
	a.depends_facilities[d.name.GetString()] = d.value.GetFloat();
      }
    }

    if (i -> value.HasMember("depends technologies")) {
      for (auto &d : i -> value["depends technologies"]) a.depends_techs.insert(d.GetFloat());
    }

    data[a.name] = a;
  }

  init = true;
  return data;
}
    
facility_object::facility_object(){
  level = 0;
}

list<string> development_tree::available() {
  throw exception();
}
