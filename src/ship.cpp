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

void ship::interact(game_data *g){
  fleet::ptr f = g -> get_fleet(fleet_id);
  string action = f -> com.action;
  
  // check land
  if (identifier::get_type(f -> com.target) == solar::class_id && action == fleet_action::go_to && f -> converge){
    solar::ptr s = g -> get_solar(f -> com.target);
    if (utility::l2d2(s -> position - position) < pow(s -> radius, 2)){
      fleet_id = identifier::source_none;
      s -> ships.insert(id);
      f -> remove_ship(id);
    }
  }

  // check ship interactions
  for (auto inter : compile_interactions()){
    // let fleet behaviour decide whether to perform the action
    if (f -> confirm_ship_interaction(inter)){
      interaction i = interaction::table()[inter];
      list<combid> buf = g -> search_targets(position, current_stats.interaction_radius, i.condition.owned_by(owner));
      if (!buf.empty()){
	combid target = utility::uniform_sample(buf);
	i.perform(ptr(this), g -> entity[target]);
      }
    }
  }
}

void ship::post_phase(game_data *g){}

void ship::receive_damage(game_object::ptr from, float damage){
  current_stats.hp -= damage;
  remove = current_stats.hp <= 0;
}

void ship::on_remove(game_data *g){
  g -> get_fleet(fleet_id) -> remove_ship(id);
  game_object::on_remove(g);
}

ship::ptr ship::create(){
  return ptr(new ship());
}

ship::ptr ship::clone(){
  return dynamic_pointer_cast<ship>(clone_impl());
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
