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

ship::ship(){}

ship::~ship(){}

void ship::pre_phase(game_data *g){
  // load weapons
  load = fmin(load + 1, current_stats.load_time);
}

void ship::move(game_data *g){
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
  current_stats.hp -= damage;
  remove = current_stats.hp <= 0;
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
  return current_stats.vision;
}

set<string> ship::compile_interactions(){
  set<string> res;
  auto utab = upgrade::table();
  for (auto v : upgrades) res += utab[v].inter;
  return res;
}

bool ship::confirm_interaction(string a, combid t, game_data *g) {
  return g -> get_fleet(fleet_id) -> confirm_ship_interaction(a, t);
}

float ship::interaction_radius() {
  return current_stats.interaction_radius;
}

bool ship::is_active(){
  return fleet_id != identifier::source_none;
}

ship_stats ship_stats::operator+= (const ship_stats &b) {
  speed += b.speed;
  hp += b.hp;
  accuracy += b.accuracy;
  ship_damage += b.ship_damage;
  solar_damage += b.solar_damage;
  interaction_radius += b.interaction_radius;
  vision += b.vision;
  load_time += b.load_time;
}

ship_stats::ship_stats(){
  speed = 0;
  hp = 0;
  accuracy = 0;
  ship_damage = 0;
  solar_damage = 0;
  interaction_radius = 0;
  vision = 0;
  load_time = 0;
}

void ship::copy_from(const ship &s){
  (*this) = s;
}

bool ship::isa(string c) {
  return c == ship::class_id || c == physical_object::class_id;
}
