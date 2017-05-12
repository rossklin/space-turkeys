#include <iostream>

#include "game_data.h"
#include "ship.h"
#include "fleet.h"
#include "utility.h"
#include "upgrades.h"
#include "serialization.h"

using namespace std;
using namespace st3;

const string ship::class_id = "ship";

const hm_t<string, ship_stats>& ship_stats::table(){
  static bool init = false;
  static hm_t<string, ship_stats> buf;

  if (init) return buf;

  auto pdoc = utility::get_json("ship");
  auto &doc = (*pdoc)["ship"];

  ship_stats s, a;
  s.speed = 1;
  s.vision_range = 50;
  s.hp = 1;
  s.ship_damage = 0;
  s.solar_damage = 0;
  s.accuracy = 0;
  s.interaction_radius_value = 20;
  s.load_time = 50;
  s.ship_class = "";
  s.upgrades.insert(interaction::land);
  s.depends_tech = "";
  s.depends_facility_level = 0;
  s.cargo_capacity = 0;

  // read ships from json structure
  for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); i++) {
    a = s;
    if (i -> value.HasMember("speed")) a.speed = i -> value["speed"].GetDouble();
    if (i -> value.HasMember("vision")) a.vision_range = i -> value["vision"].GetDouble();
    if (i -> value.HasMember("hp")) a.hp = i -> value["hp"].GetDouble();
    if (i -> value.HasMember("ship_damage")) a.ship_damage = i -> value["ship_damage"].GetDouble();
    if (i -> value.HasMember("solar_damage")) a.solar_damage = i -> value["solar_damage"].GetDouble();
    if (i -> value.HasMember("accuracy")) a.accuracy = i -> value["accuracy"].GetDouble();
    if (i -> value.HasMember("interaction_radius")) a.interaction_radius_value = i -> value["interaction_radius"].GetDouble();
    if (i -> value.HasMember("load_time")) a.load_time = i -> value["load_time"].GetDouble();
    if (i -> value.HasMember("cargo_capacity")) a.cargo_capacity = i -> value["cargo_capacity"].GetDouble();
    if (i -> value.HasMember("depends_tech")) a.depends_tech = i -> value["depends_tech"].GetString();
    if (i -> value.HasMember("depends_facility_level")) a.depends_facility_level = i -> value["depends_facility_level"].GetInt();
      
    a.ship_class = i -> name.GetString();

    if (i -> value.HasMember("upgrades")) {
      if (!i -> value["upgrades"].IsArray()) {
	throw runtime_error("Invalid upgrades array in ship_data.json!");
      }

      auto &upgrades = i -> value["upgrades"];
      for (auto u = upgrades.Begin(); u != upgrades.End(); u++) a.upgrades.insert(u -> GetString());
    }

    if (i -> value.HasMember("cost")) {
      auto &cost = i -> value["cost"];
      for (auto k : keywords::resource) {
	const char *c = k.c_str();
	if (cost.HasMember(c)) a.build_cost[k] = cost[c].GetDouble();
      }
      if (cost.HasMember("time")) a.build_time = cost["time"].GetDouble();
    }

    if (i -> value.HasMember("shape")) {
      auto &shape = i -> value["shape"];
      pair<point, char> v;
      for (auto j = shape.Begin(); j != shape.End(); j++) {
	v.first.x = (*j)["x"].GetDouble();
	v.first.y = (*j)["y"].GetDouble();
	v.second = (*j)["c"].GetString()[0];
	a.shape.push_back(v);
      }
    }
    
    buf[a.ship_class] = a;
  }

  delete pdoc;
  init = true;
  return buf;
}

ship::ship(){}

ship::ship(const ship_stats &s) : ship_stats(s) {
  base_stats = s;
  fleet_id = identifier::source_none;
  remove = false;
  load = 0;
  passengers = 0;
  is_landed = false;
}

ship::~ship(){}

list<string> ship::all_classes() {
  return utility::get_map_keys(table());
}

void ship::set_stats(ship_stats s){
  static_cast<ship_stats&>(*this) = s;
}

void ship::pre_phase(game_data *g){
  // load weapons
  load = fmin(load + 1, base_stats.load_time);
}

void ship::move(game_data *g){
  if (!has_fleet()) return;
  fleet::ptr f = g -> get_fleet(fleet_id);

  // check fleet is not idle
  if (!f -> is_idle()){
    point delta;
    if (f -> converge){
      delta = f -> target_position - position;
    }else{
      delta = f -> target_position - f -> position;
    }

    float target_angle = utility::point_angle(delta);
    float angle_increment = 0.1;
    float epsilon = 0.01;
    float angle_sign = utility::signum(utility::angle_difference(target_angle, angle), epsilon);

    angle += angle_increment * angle_sign;
    position = position + utility::scale_point(utility::normv(angle), f -> speed_limit);
    g -> entity_grid -> move(id, position);
  }
}

void ship::post_phase(game_data *g){}

void ship::receive_damage(game_object::ptr from, float damage){
  hp -= damage;
  remove = hp <= 0;
  cout << "ship::receive_damage: " << id << " takes " << damage << " damage from " << from -> id << " - remove = " << remove << endl;
}

void ship::on_remove(game_data *g){
  if (g -> entity.count(fleet_id)) {
    g -> get_fleet(fleet_id) -> remove_ship(id);
  }
  game_object::on_remove(g);
}

ship::ptr ship::create(){
  return ptr(new ship());
}

ship::ptr ship::clone(){
  return dynamic_cast<ship::ptr>(clone_impl());
}

game_object::ptr ship::clone_impl(){
  return ptr(new ship(*this));
}

bool ship::serialize(sf::Packet &p){
  return p << class_id << *this;
}

float ship::vision(){
  return vision_range;
}

set<string> ship::compile_interactions(){
  set<string> res;
  auto utab = upgrade::table();
  for (auto v : upgrades) res += utab[v].inter;
  return res;
}

list<combid> ship::confirm_interaction(string a, list<combid> targets, game_data *g) {
  list<combid> allowed;
  if (has_fleet()){
    for (auto t : targets) {
      if (g -> get_fleet(fleet_id) -> confirm_ship_interaction(a, t)){
	allowed.push_back(t);
      }
    }
  }else{
    if (a == interaction::space_combat) {
      allowed = targets;
    }
  }

  // chose at most one target
  list<combid> result;
  if (!allowed.empty()) result.push_back(utility::uniform_sample(allowed));
  return result;
}

bool ship::accuracy_check(float a, ship::ptr t) {
  return utility::random_uniform() < (cos(a) + 1) / 2;
}

float ship::interaction_radius() {
  return interaction_radius_value;
}

bool ship::is_active(){
  return !is_landed;
}

bool ship::has_fleet() {
  return fleet_id != identifier::source_none;
}

void ship::on_liftoff(solar::ptr from, game_data *g){
  g -> players[owner].research_level.repair_ship(*this, from);
  is_landed = false;

  for (auto v : upgrades) {
    upgrade u = upgrade::table().at(v);
    for (auto i : u.on_liftoff) interaction::table().at(i).perform(this, from, g);
  }
}

void ship_stats::operator+= (const ship_stats &b) {
  speed += b.speed;
  hp += b.hp;
  accuracy += b.accuracy;
  ship_damage += b.ship_damage;
  solar_damage += b.solar_damage;
  interaction_radius_value += b.interaction_radius_value;
  vision_range += b.vision_range;
  load_time += b.load_time;
  cargo_capacity += b.cargo_capacity;
  upgrades += b.upgrades;
  build_cost.add(b.build_cost);
  build_time += b.build_time;
}

ship_stats::ship_stats(){
  speed = 0;
  hp = 0;
  accuracy = 0;
  ship_damage = 0;
  solar_damage = 0;
  interaction_radius_value = 0;
  vision_range = 0;
  load_time = 0;
  cargo_capacity = 0;
  depends_facility_level = 0;
  build_time = 0;
}

void ship::copy_from(const ship &s){
  (*this) = s;
}

bool ship::isa(string c) {
  return c == ship::class_id || c == physical_object::class_id;
}
