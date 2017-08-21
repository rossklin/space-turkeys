#include <vector>
#include <iostream>
#include <rapidjson/document.h>
#include <mutex>
#include <sstream>

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
const float solar::f_growth = 4e-2;
const float solar::f_crowding = 4e-3;
const float solar::f_minerate = 4e-4;
const float solar::f_buildrate = 2e-3;
const float solar::f_devrate = 2e-3;
const float solar::f_resrate = 2e-3;

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

  // select ship class for production
  if (next_ship.empty()) {
    stringstream ss;
    ss << id << ": selecting next_ship: " << endl;
    cost::ship_allocation test = g -> players[owner].military;
    for (auto &x : test.data) {
      ss << x.first << ": " << x.second;
      if (!research_level -> can_build_ship(x.first, ptr(this))) {
	x.second = 0;
	ss << " (unset)";
      }
      ss << endl;
    }
    
    next_ship = utility::weighted_sample(test.data);
    ss << "chose: " << next_ship << endl;
    if (next_ship.size()) server::output(ss.str(), true);
  }
  
  dynamics();

  // build ships
  for (auto v : ship::all_classes()) {
    if (research_level -> can_build_ship(v, ptr(this))){
      float build_time = ship::table().at(v).build_time;
      while (fleet_growth[v] >= build_time) {
	next_ship.clear();
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

  s -> radius = 10 + 7 * sqrt(s -> available_resource.count() / 3000);
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

float solar::base_growth() {
  return population * happiness * f_growth;
}

float solar::population_increment(){
  static float rate = 0.2;
  float culture_growth = base_growth() * compute_boost(keywords::key_culture);
  return rate * (base_growth() + culture_growth - crowding_rate());
}

float solar::ecology_increment(){
  float env_factor = space_status() * water_status();
  return 0.01 * (compute_boost(keywords::key_ecology) * env_factor - ecology);
}

float solar::happiness_increment(choice::c_solar &c){
  float crowding_factor = 3 * crowding_rate() / population; // < 0.28
  float size_factor = 0.5 * log(population) / (2 * ecology + 1); // < 0.02
  float pacifist_factor = 0.5 * c.allocation[keywords::key_military]; // 0.2ish?
  float culture_factor = 10 * pow(compute_boost(keywords::key_culture), 2) * c.allocation[keywords::key_culture]; // 0.3ish?
  float reg_factor = 0.3 * (happiness - 0.5); // 0.1ish?

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

float solar::ship_increment(choice::c_solar &c){
  return f_buildrate * compute_boost(keywords::key_military) * c.allocation[keywords::key_military] * compute_workers();
}

float solar::compute_workers(){
  return happiness * population;
}

choice::c_solar solar::government() {
  choice::c_solar c = choice_data;
  
  if (!utility::find_in(c.governor, keywords::governor)) {
    throw runtime_error("Invalid governor: " + c.governor);
  }
  
  // general stuff for all governors
  for (auto v : keywords::governor) c.allocation[v] = 1;

  // MINING
  auto compute_mining = [this] (choice::c_solar c) -> choice::c_solar {
    float rsum = 0;
    float ssum = 0;
    float smin = INFINITY;
    for (auto v : keywords::resource) {
      rsum += available_resource[v];
      ssum += resource_storage[v];
      smin = fmin(smin, resource_storage[v]);
      if (c.governor == keywords::key_mining) {
	// mine what is available
	c.mining[v] = available_resource[v];
      } else {
	// mine what is needed
	c.mining[v] = 1 / (resource_storage[v] + 0.1);
      }
    }

    if (c.governor == keywords::key_mining) {
      // always mine
      c.allocation[keywords::key_mining] = 4;
    } else {
      // mine if storage is running low
      c.allocation[keywords::key_mining] = 4 / (smin + 0.1);
    }
  
    if (!rsum) c.allocation[keywords::key_mining] = 0;
    return c;
  };

  c = compute_mining(c);

  // RESEARCH
  if (c.governor == keywords::key_research) c.allocation[keywords::key_research] = 2;
  if (c.governor == keywords::key_mining) c.allocation[keywords::key_research] = 0;
  if (research_level -> researching.empty()) c.allocation[keywords::key_research] = 0;

  float care_factor = 1;
  if (c.governor == keywords::key_mining || c.governor == keywords::key_military) {
    care_factor = 0.5;
  } else if (c.governor == keywords::key_culture) {
    care_factor = 2;
  }

  // DEVELOPMENT
  auto select_development = [this, care_factor] (choice::c_solar c) -> choice::c_solar {
    bool is_developing = c.development.size() > 0;
    if (is_developing) return c;
    
    hm_t<string, float> score;
    cost::res_t total = available_resource;
    total.add(resource_storage);

    // base score for facilities from relevant sector boost
    for (auto v : available_facilities(*research_level)) {
      facility_object test = developed(v, 1);
      float h = 0.1;

      auto add_factor = [test] (string key, float w) -> float {
	if (test.sector_boost.count(key)) {
	  return w * (test.sector_boost.at(key) - 1);
	} else {
	  return 0;
	}
      };

      // score for favorite sector boost
      h += add_factor(c.governor, 3);

      // score for culture, medecine and ecology if needed
      h += add_factor(keywords::key_culture, care_factor * pow(fmax(1 - happiness, 0), 0.5));
      h += add_factor(keywords::key_ecology, care_factor * pow(fmax(1 - ecology, 0), 0.3));
      h += add_factor(keywords::key_medicine, care_factor * pow(crowding_rate() / base_growth(), 0.3));

      // reduce score for build time
      h *= 4 / log(test.cost_time + test.cost_resources.count() + 1);

      // significantly reduce score if there are not sufficient resources
      for (auto u : keywords::resource) {
	if (total[u] < test.cost_resources[u]) {
	  h /= 10;
	  break;
	}
      }
    
      score[v] = h;
    }

    auto add_score = [&score] (string name, float value) {
      if (score.count(name)) score[name] += value;
    };

    // special score for shipyard and research facility
    add_score("shipyard", 0.2);
    add_score("research facility", 0.2);
    if (c.governor == keywords::key_military) add_score("shipyard", 0.4);
    if (c.governor == keywords::key_research) add_score("research facility", 0.4);

    // add score for doing nothing
    score[""] = 0.05;
    if (c.governor == keywords::key_mining) score[""] = 10;

    // weighted probability select
    c.development = utility::weighted_sample(score);

    // prioritize development if score is good
    if (c.development.size()) {
      float devprio = score[c.development];

      if (c.governor == keywords::key_development) {
	devprio *= 2;
      } else if (c.governor == keywords::key_military || c.governor == keywords::key_mining) {
	devprio *= 0.5;
      }
      
      c.allocation[keywords::key_development] = devprio;
    } else {
      c.allocation[keywords::key_development] = 0;
    }

    stringstream print;
    print << "Development choice for " << id << " with " << c.governor << " governor:" << endl;
    print << "Scores: " << endl;
    for (auto x : score) print << " - " << x.first << ": " << x.second << endl;
    print << "Choice: " << c.development << " (allocation = " << c.allocation[keywords::key_development] << ")" << endl;

    server::output(print.str(), true);
    
    return c;
  };
  
  c = select_development(c);

  // MILITARY
  float militarist_factor = c.governor == keywords::key_military;
  c.allocation[keywords::key_military] = (!next_ship.empty()) * (0.5 + militarist_factor);

  // normalize
  c = c.normalize();

  return c;
}

void solar::dynamics(){
  choice_data = government();
  choice::c_solar c = choice_data;
  
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
    if (c.development.size()) {
      add_cost("dev", development_increment(c) * dt, buf.facility_access(c.development) -> cost_time, developed(c.development, 1).cost_resources);
    }
    
    // military industry
    if (next_ship.size()) {
      add_cost("ship:" + next_ship, ship_increment(c) * dt, ship::table().at(next_ship).build_time, ship::table().at(next_ship).build_cost);
    }

    // compute allowed production ratio
    float allowed = fmin(1, resource_constraint(total_cost));
    if (allowed < 1) buf.out_of_resources = true;

    // add production for ships
    if (next_ship.size()) {
      buf.fleet_growth[next_ship] += allowed * weight_table["ship:" + next_ship];
    }

    // add production for development
    if (c.development.size()) {
      buf.facility_access(c.development) -> progress += allowed * weight_table["dev"];
    }

    // pay for production
    total_cost.scale(allowed);
    buf.pay_resources(total_cost);
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
  base.vision *= pow(1.3, level);
  base.turret.range *= level;
  base.turret.damage *= level;
  base.shield *= level;
  base.water_provided *= level;
  base.space_provided *= level;
  base.water_usage *= level;
  base.space_usage *= level;

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
