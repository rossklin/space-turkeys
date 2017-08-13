#include <vector>
#include <iostream>
#include <rapidjson/document.h>
#include <mutex>

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

solar::solar(const solar &s) : game_object(s) {
  *this = s;
};

void solar::pre_phase(game_data *g){
  out_of_resources = false;
  
  research_level = &g -> players[owner].research_level;
  for (auto t : facility_access()){
    if (t -> is_turret){
      t -> turret.load += 0.02 * t -> level;
    }
  }
}

// so far, solars don't move
void solar::move(game_data *g){
  if (owner < 0 || population <= 0) return;

  dt = g -> get_dt();
  dynamics();

  // build ships
  for (auto v : ship::all_classes()) {
    if (research_level -> can_build_ship(v, ptr(this))){
      float build_time = ship::table().at(v).build_time;
      while (fleet_growth[v] >= build_time) {
	fleet_growth[v] -= build_time;
	ship sh = research_level -> build_ship(v, ptr(this));
	sh.is_landed = true;
	sh.owner = owner;
	ships.insert(sh.id);
	g -> add_entity(ship::ptr(new ship(sh)));
      }
    }
  }

  // build facilities
  if (choice_data.development.length() > 0) {
    string dev = choice_data.development;
    if (list_facility_requirements(dev, *research_level).empty()) {
      if (develop(dev)) {
	g -> players[owner].log.push_back("Completed building " + dev);
	choice_data.development = "";
      }
    }
  }

  // check for turret combat interaction
  target_condition cond(target_condition::enemy, ship::class_id);
  list<combid> buf = g -> search_targets(id, position, interaction_radius(), cond.owned_by(owner));

  if (!buf.empty()) {
    combid t = utility::uniform_sample(buf);
    interaction_info info;
    info.source = id;
    info.target = t;
    info.interaction = interaction::turret_combat;
    g -> interaction_buffer.push_back(info);
  }
}

set<string> solar::compile_interactions(){
  return {interaction::turret_combat};
}

float solar::interaction_radius() {
  float r = 0;

  for (auto t : developed()){
    if (t.is_turret){
      r = max(r, t.turret.range);
    }
  }
  
  return r;
}

void solar::receive_damage(game_object::ptr s, float damage, game_data *g){
  damage -= compute_shield_power();
  g -> log_bombard(s -> id, id);
  
  if (damage <= 0) return;
  
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
    for (auto f : facility_access()) {
      if (f -> hp < f -> base_hp) f -> hp += 0.01;
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
  for (auto x : developed()) res = fmax(res, x.vision);
  return res + radius;
}

solar::ptr solar::create(point p, float bounty) {
  static int idc = 0;
  static mutex m;
  auto fres = [bounty] () {
    return fmax(utility::random_normal(pow(2, 10 * bounty), pow(2, 8 * bounty)), 0);
  };

  solar::ptr s = new solar();

  m.lock();
  s -> id = identifier::make(solar::class_id, idc++);
  m.unlock();

  s -> population = 0;
  s -> happiness = 1;
  s -> research_points = 0;
    
  s -> water = fres();
  s -> space = fres();
  s -> ecology = fmax(fmin(utility::random_normal(0.5, 0.2), 1), 0.2);

  for (auto v : keywords::resource) s -> available_resource[v] = fres();

  s -> radius = 10 + 10 * s -> available_resource.count() / 3000;
  s -> position = p;
  s -> owner = game_object::neutral_owner;
  s -> was_discovered = false;

  return s;
}

game_object::ptr solar::clone(){
  return new solar(*this);
}

bool solar::serialize(sf::Packet &p){
  return p << class_id << *this;
}

bool solar::has_defense(){
  if (owner == game_object::neutral_owner) return false;
  for (auto t : facility_access()) if (t -> is_turret) return true;
}

void solar::damage_facilities(float d){
  float d0 = d;
  list<string> remove;
  while (d > 0 && facility_access().size() > 0){
    remove.clear();
    for (auto i : facility_access()) {
      if (i -> level == 0) continue;
      if (d <= 0) break;
      
      float k = fmin(utility::random_uniform(0, 0.1 * d0), d);
      d -= k;
      if (i -> hp > 0) {
	i -> hp -= k;
	if (i -> hp <= 0) remove.push_back(i -> name);
      } else {
	if (utility::random_uniform(-(i -> level), k) > 0) remove.push_back(i -> name);
      }
    }
    for (auto r : remove) development[r] = facility_table().at(r);
  }
}

// [0,1] evaluation of available free space
float solar::space_status(){
  if (space <= 0) return 0;
  
  float used = 0;
  for (auto t : developed()) {
    used += t.space_usage;
    used -= t.space_provided;
  }

  if (used > space) throw runtime_error("space_status: used more than space");
  return fmin((space - used) / space, 1);
}

// [0,1] evaluation of available water
float solar::water_status(){
  if (water <= 0) return 0;
  
  float used = 0;
  for (auto t : developed()) {
    used += t.water_usage;
    used -= t.water_provided;
  }

  if (used > water) throw runtime_error("water status: used more than water!");
  return fmin((water - used) / water, 1);
}

float solar::crowding_rate() {
  float env_factor = ecology * space * space_status();
  return f_crowding * pow(population, 2) / (compute_boost(keywords::key_medicine) * env_factor + 1);
}

float solar::population_increment(){
  static float rate = 0.2;
  float base_growth = population * happiness * f_growth;
  float culture_growth = base_growth * compute_boost(keywords::key_culture);
  return rate * (base_growth + culture_growth - crowding_rate());
}

float solar::ecology_increment(){
  float env_factor = space_status() * water_status();
  return 0.01 * (compute_boost(keywords::key_ecology) * env_factor - ecology);
}

float solar::happiness_increment(choice::c_solar &c){
  float crowding_factor = crowding_rate() / population;
  float size_factor = 0.05 * log(population) / (2 * ecology + 1);
  float pacifist_factor = c.allocation[keywords::key_military];
  float culture_factor = 0.3 * compute_boost(keywords::key_culture) + c.allocation[keywords::key_culture];
  float reg_factor = 0.3 * (happiness - 0.5);

  return 0.01 * (reg_factor + culture_factor - crowding_factor - size_factor - pacifist_factor);
}

float solar::research_increment(choice::c_solar &c){
  return f_resrate * c.allocation[keywords::key_research] * compute_boost(keywords::key_research) * compute_workers();
}

float solar::resource_increment(string v, choice::c_solar &c){
  return f_minerate * compute_boost(keywords::key_mining) * c.allocation[keywords::key_mining] * c.mining[v] * compute_workers();
}

float solar::development_increment(choice::c_solar &c){
  return f_devrate * c.allocation[keywords::key_development] * compute_boost(keywords::key_development) * compute_workers();
}

float solar::ship_increment(string v, choice::c_solar &c){
  return f_buildrate * compute_boost(keywords::key_military) * c.allocation[keywords::key_military] * c.military[v] * compute_workers();
}

float solar::compute_workers(){
  return happiness * population;
}

void solar::dynamics(){
  // disable mining if there are no resources
  // mine more if there is less in storage
  float rsum = 0;
  float ssum = 0;
  for (auto v : keywords::resource) {
    rsum += available_resource[v];
    ssum += resource_storage[v];
    choice_data.mining[v] = 1 / (resource_storage[v] + 1);
  }
  if (!rsum) choice_data.allocation[keywords::key_mining] = 0;
  
  choice::c_solar c = choice_data;

  // disable development/research if none selected
  bool is_developing = !c.development.empty();
  if (!is_developing) c.allocation[keywords::key_development] = 0;
  if (research_level -> researching.empty()) c.allocation[keywords::key_research] = 0;
  
  c.normalize();
  
  solar buf = *this;
  float dw = sqrt(dt);

  // ecological development
  buf.ecology += ecology_increment() * dt + utility::random_normal(0, 0.01) * dw;
  buf.ecology = fmax(fmin(buf.ecology, 1), 0);

  if (population > 0){
    // population development    
    float random_growth = population * f_growth * utility::random_normal(0, 0.2);
    buf.population = fmax(population + population_increment() * dt + random_growth * dw, 0);

    // happiness development
    buf.happiness += happiness_increment(c) * dt + utility::random_normal(0, 0.01) * dw;
    buf.happiness = fmax(fmin(1, buf.happiness), 0);

    // research and development
    buf.research_points += research_increment(c) * dt;

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

    // sum resource and time costs
    auto add_cost = [&total_quantity, &total_cost, &weight_table] (string key, float quantity, float build_time, cost::res_t build_cost) {
      build_cost.scale(quantity / build_time);
      total_quantity += quantity;
      total_cost.add(build_cost);
      weight_table[key] = quantity;
    };

    // development
    if (is_developing) {
      add_cost("dev", development_increment(c) * dt, buf.facility_access(c.development) -> cost_time, developed(c.development, 1).cost_resources);
    }
    
    // military industry
    for (auto v : ship::all_classes()) {
      add_cost("ship:" + v, ship_increment(v, c) * dt, ship::table().at(v).build_time, ship::table().at(v).build_cost);
    }

    // compute allowed production ratio
    float allowed = fmin(1, resource_constraint(total_cost));
    if (allowed < 1) buf.out_of_resources = true;

    // add production for ships
    for (auto v : ship::all_classes()) {
      buf.fleet_growth[v] += allowed * weight_table["ship:" + v];
    }

    // add production for development
    if (is_developing) {
      buf.facility_access(c.development) -> progress += allowed * weight_table["dev"];
    }

    // pay for production
    total_cost.scale(allowed);
    buf.pay_resources(total_cost);

    // adaptive mining
    float m = buf.choice_data.allocation[keywords::key_mining];

    if (buf.out_of_resources) {
      if (m < 1) {
	m = 1;
      } else {
	m *= 1.2;
      }
    } else {
      if (m > 1) {
	m = fmax(0.9 * m, 1);
      }
    }

    buf.choice_data.allocation[keywords::key_mining] = m;
  }

  *this = buf;
}

bool solar::isa(string c) {
  return c == solar::class_id || c == physical_object::class_id || c == commandable_object::class_id;
}

bool solar::can_see(game_object::ptr x) {
  float r = vision();
  if (!x -> is_active()) return false;

  if (x -> isa(ship::class_id)) {
    ship::ptr s = utility::guaranteed_cast<ship>(x);
    float area = pow(s -> stats[sskey::key::mass], 2 / (float)3);
    r = vision() * fmin(area / (s -> stats[sskey::key::stealth] + 1), 1);
  }
  
  float d = utility::l2norm(x -> position - position);
  return d < r;
}

float solar::compute_shield_power() {
  float sum = 0;
  for (auto f : developed()) sum += f.shield;
  return log(sum + 1);
}

float solar::compute_boost(string v){
  float sum = 1;
  for (auto f : developed()) {
    if (f.sector_boost.count(v)) {
      sum *= f.sector_boost[v];
    }
  }

  auto &rtab = research::data::table();
  for (auto r : research_level -> researched()) {
    research::tech &t = research_level -> access(r);
    if (t.sector_boost.count(v)) sum *= t.sector_boost[v];
  }
  
  return sum;
}

bool solar::develop(string fac) {
  facility_object f = developed(fac, 1);
  if (f.progress < f.cost_time) return false;

  // reset progress
  facility_object *fx = facility_access(fac);
  fx -> progress = 0;

  // level up and repair
  fx -> level++;
  fx -> hp = fx -> base_hp;
  return true;
}

facility_object solar::developed(string key, int lev_inc) {  
  facility_object base = *facility_access(key);
  int level = max((int)base.level + lev_inc, (int)0);
  float lm = cost::expansion_multiplier(level);

  // scale costs
  base.cost_time *= lm;
  base.cost_resources.scale(lm);

  // scale boosts
  base.vision *= pow(1.5, level);
  base.turret.range *= level;
  base.turret.damage *= level;
  base.shield *= level;
  base.water_provided *= level;
  base.space_provided *= level;

  for (auto &x : base.sector_boost) x.second = pow(x.second, level);
  base.level = level;

  return base;
}

list<facility_object> solar::developed() {
  list<facility_object> d;

  for (auto &x : development) {
    if (x.second.level == 0) continue;
    facility_object f = developed(x.first);
    d.push_back(f);
  }
  
  return d;
}

facility_object *solar::facility_access(string key) {
  if (!development.count(key)) development[key] = facility_table().at(key);
  return &development[key];
}

list<facility_object*> solar::facility_access() {
  list<facility_object*> res;

  for (auto &x : development) {
    if (x.second.level > 0) res.push_back(&x.second);
  }

  return res;
}

void facility::read_from_json(const rapidjson::Value &x) {

  for (auto i = x.MemberBegin(); i != x.MemberEnd(); i++) {
    string name = i -> name.GetString();
    if (development::node::parse(name, i -> value)) continue;

    bool success = false;
    if (i -> value.IsObject()) {
      if (name == "turret"){
	is_turret = 1;
	for (auto t = i -> value.MemberBegin(); t != i -> value.MemberEnd(); t++) {
	  string t_name = t -> name.GetString();
	  if (t_name == "damage"){
	    turret.damage = t -> value.GetDouble();
	  }else if (t_name == "range"){
	    turret.range = t -> value.GetDouble();
	  }else if (t_name == "accuracy"){
	    turret.accuracy = t -> value.GetDouble();
	  }else if (t_name == "load"){
	    turret.load = t -> value.GetDouble();
	  }else{
	    throw runtime_error("Failed to parse turret stats: " + t_name);
	  }
	}
	success = true;

      }else if (name == "cost"){
	for (auto t = i -> value.MemberBegin(); t != i -> value.MemberEnd(); t++) {
	  if (!cost::parse_resource(t -> name.GetString(), t -> value.GetDouble(), cost_resources)) {
	    throw runtime_error("Failed to parse facility cost: " + string(t -> name.GetString()));
	  }
	}
	success = true;
      }
    } else {
      float value = i -> value.GetDouble();
      if (name == "vision"){
	vision = value;
	success = true;
      }else if (name == "hp"){
	base_hp = value;
	success = true;
      }else if (name == "shield"){
	shield = value;
	success = true;
      }else if (name == "water usage"){
	water_usage = value;
	success = true;
      }else if (name == "space usage"){
	space_usage = value;
	success = true;
      }else if (name == "water provided"){
	water_provided = value;
	success = true;
      }else if (name == "space provided"){
	space_provided = value;
	success = true;
      }
    }

    if (!success) {
      throw runtime_error("Failed to parse facility: " + name);
    }
  }
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
  hp = 0;
}
    
facility_object::facility_object(const facility &f) : facility(f) {
  hp = 0;
}

facility::facility() : development::node() {
  vision = 0;
  is_turret = 0;
  base_hp = 0;
  shield = 0;
  water_usage = 0;
  space_usage = 0;
  water_provided = 0;
  space_provided = 0;
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

float turret_t::accuracy_check(ship::ptr t, float d) {
  return accuracy * accuracy_distance_norm / (d + 1);
}

list<string> solar::list_facility_requirements(string v, const research::data &r_level) {
  list<string> req;
  facility f = developed(v, 1);

  // check cost
  if (water * water_status() < f.water_usage) req.push_back("water");
  if (space * space_status() < f.space_usage) req.push_back("space");

  // check dependencies
  for (auto &y : f.depends_facilities) if (facility_access(y.first) -> level < y.second) req.push_back(y.first + " level " + to_string(y.second));
  for (auto &y : f.depends_techs) if (!r_level.researched().count(y)) req.push_back("technology " + y);

  return req;
}

list<string> solar::available_facilities(const research::data &r_level) {
  list<string> r;
  for (auto &x : facility_table()) if (list_facility_requirements(x.first, r_level).empty()) r.push_back(x.first);
  return r;
}
