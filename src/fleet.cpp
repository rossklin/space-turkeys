#include <iostream>

#include "fleet.h"
#include "ship.h"
#include "utility.h"
#include "game_data.h"
#include "serialization.h"
#include "upgrades.h"

using namespace std;
using namespace st3;

const string fleet::class_id = "fleet";

const string fleet_action::land = "land";
const string fleet_action::turret_combat = "turret combat";
const string fleet_action::space_combat = "space combat";
const string fleet_action::bombard = "bombard";
const string fleet_action::colonize = "colonize";
const string fleet_action::go_to = "go to";
const string fleet_action::idle = "idle";

set<string> fleet::all_interactions(){
  return {fleet_action::land, fleet_action::turret_combat, fleet_action::space_combat, fleet_action::bombard, fleet_action::colonize};
}

set<string> fleet::all_base_actions(){
  return {fleet_action::go_to};
}

hm_t<string, target_condition> &fleet::action_condition_table(){
  static hm_t<string, target_condition> data;
  static bool init = false;

  if (!init){
    init = true;

    // interaction actions
    auto itab = interaction::table();
    for (auto a : all_interactions()) data[a] = itab[a].condition;

    // base actions
    data[fleet_action::go_to] = target_condition(target_condition::owned, waypoint::class_id);
    data[fleet_action::idle] = target_condition(target_condition::any_alignment, target_condition::no_target);
  }

  return data;
}


fleet::fleet(){
  static int idc = 0;
  id = identifier::make(class_id, idc++);

  position = point(0,0);
  radius = 0;
  update_counter = 0;
  owner = -1;
  converge = false;
  speed_limit = 0;
  remove = false;
}

fleet::~fleet(){}

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

fleet::ptr fleet::create(){
  return ptr(new fleet());
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
      ptr f = create();
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

  // check target status valid
  target_condition c = fleet::action_condition_table()[com.action];
  if (!c.owned_by(owner).valid_on(g -> get_entity(com.target))) {
    cout << "target " << com.target << " no longer valid for " << id << endl;
    set_idle();
  }

  // have arrived?
  if (!is_idle()){
    if (g -> target_position(com.target, target_position)) converge = utility::l2d2(target_position - position) < fleet::interact_d2;
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
  if (a == fleet_action::space_combat) return true;

  bool action_match = a == com.action;
  bool target_match = t == com.target;

  // allow "land" on target solar if action is go_to
  if (a == fleet_action::land && com.action == fleet_action::go_to) return target_match;

  // allow colonize and bombard only on correct target
  if (a == fleet_action::bombard || a == fleet_action::colonize) {
    return action_match && target_match;
  }

  // else, only allow the active fleet command action
  return action_match;
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
