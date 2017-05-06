#include <iostream>

#include "interaction.h"
#include "fleet.h"
#include "ship.h"
#include "utility.h"
#include "game_data.h"
#include "serialization.h"
#include "upgrades.h"

using namespace std;
using namespace st3;

const string fleet::class_id = "fleet";

const string fleet_action::go_to = "go to";
const string fleet_action::idle = "idle";

fleet::fleet(){}

fleet::fleet(idtype pid){
  static int idc = 0;
  if (pid < 0){
    id = identifier::make(class_id, "S#" + to_string(idc++));
  }else{
    id = identifier::make(class_id, to_string(pid) + "#" + to_string(idc++));
  }

  position = point(0,0);
  radius = 0;
  update_counter = 0;
  owner = -1;
  converge = false;
  speed_limit = 0;
  remove = false;
  com.action = fleet_action::idle;
}

fleet::~fleet(){}

void fleet::on_remove(game_data *g) {
  for (auto sid : ships) {
    g -> get_ship(sid) -> fleet_id = identifier::source_none;
  }
  game_object::on_remove(g);
}

void fleet::pre_phase(game_data *g){
  check_in_sight(g);
  update_data(g);
  check_waypoint(g);
}

void fleet::move(game_data *g){
  // linear extrapolation of position estimate
  position += speed_limit * utility::normv(utility::point_angle(target_position - position));
}

void fleet::post_phase(game_data *g){}

float fleet::vision(){
  return vision_buf;
}

fleet::ptr fleet::create(idtype pid){
  return ptr(new fleet(pid));
}

fleet::ptr fleet::clone(){
  return dynamic_cast<fleet::ptr>(clone_impl());
}

game_object::ptr fleet::clone_impl(){
  return ptr(new fleet(*this));
}

bool fleet::serialize(sf::Packet &p){
  return p << class_id << *this;
}

bool st3::fleet::is_idle(){
  return com.action == fleet_action::idle;
}

void fleet::set_idle(){
  com.target = identifier::target_idle;
  com.action = fleet_action::idle;
}

void fleet::give_commands(list<command> c, game_data *g){
  vector<command> buf(c.begin(), c.end());
  random_shuffle(buf.begin(), buf.end());

  for (auto &x : buf){
    if (x.ships == ships){
      // maintain id for trackability
      // if all ships were assigned, break.
      com.target = x.target;
      com.action = x.action;
      break;
    }else{
      ptr f = create(fleet::server_pid);
      f -> com = x;
      f -> com.source = f -> id;
      f -> position = position;
      f -> radius = radius;
      f -> owner = owner;
      for (auto i : x.ships){
	if (ships.count(i)){
	  ship::ptr buf = g -> get_ship(i);
	  buf -> fleet_id = f -> id;
	  remove_ship(i);
	  f -> ships.insert(i);
	}
      }

      g -> add_entity(f);
    }
  }
}

void fleet::update_data(game_data *g){

  // need to update fleet data?
  if (((update_counter++) % fleet::update_period)) return;
  float speed = INFINITY;
  int count;

  // position, radius, speed and vision
  point p(0,0);
  float r2 = 0;
  vision_buf = 0;

  for (auto k : ships){
    ship::ptr s = g -> get_ship(k);
    speed = fmin(speed, s -> current_stats.speed);
    r2 = fmax(r2, utility::l2d2(s -> position - position));
    vision_buf = fmax(vision_buf, s -> current_stats.vision);
    p = p + s -> position;
  }
  
  // radius = fmax(sqrt(r2), fleet::min_radius);
  radius = g -> settings.fleet_default_radius;
  speed_limit = speed;  
  position = utility::scale_point(p, 1 / (float)ships.size());

  // the below only applies if the fleet has a target
  if (com.target == identifier::target_idle) return;

  // check target status valid if action is an interaction
  auto itab = interaction::table();
  if (itab.count(com.action)) {
    target_condition c = itab[com.action].condition.owned_by(owner);
    if (!c.valid_on(g -> get_entity(com.target))) {
      cout << "target " << com.target << " no longer valid for " << id << endl;
      set_idle();
    }
  }

  // have arrived?
  if (!is_idle()){
    if (g -> target_position(com.target, target_position)) {
      converge = utility::l2d2(target_position - position) < fleet::interact_d2;
    }
  }
}

void fleet::check_waypoint(game_data *g){
  // set to idle and 'land' ships if converged to waypoint
  if (converge && identifier::get_type(com.target) == waypoint::class_id && com.action == fleet_action::go_to){
    com.action = fleet_action::idle;
    cout << "set fleet " << id << " idle target: " << com.target << endl;
  }
}

void fleet::check_in_sight(game_data *g){
  if (com.target == identifier::target_idle) return;

  bool seen = g -> entity_seen_by(com.target, owner);
  bool solar = identifier::get_type(com.target) == solar::class_id;

  // target left sight?
  if (!(seen || solar)){
    cout << "fleet " << id << " looses sight of " << com.target << endl;

    // creating a waypoint here causes id collision with waypoints
    // created on client side
    
    // // create a waypoint and reset target
    // waypoint::ptr w = waypoint::create(owner);

    // // if the target exists, set waypoint at  target position
    // g -> target_position(com.target, w -> position);
    // g -> add_entity(w);

    set_idle();
  }
}

// todo: more advanced fleet action policies
bool fleet::confirm_ship_interaction(string a, combid t){
  // always allow "space combat"
  if (a == interaction::space_combat) return true;

  bool action_match = a == com.action;
  bool target_match = t == com.target;

  return action_match && target_match;
}

void fleet::copy_from(const fleet &s){
  (*this) = s;
}

void fleet::remove_ship(combid i){
  ships.erase(i);
  remove = ships.empty();
}

bool fleet::isa(string c) {
  return c == fleet::class_id || c == commandable_object::class_id;
}
