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

const string fleet_action::space_combat = "space combat";
const string fleet_action::bombard = "bombard";
const string fleet_action::colonize = "colonize";
const string fleet_action::go_to = "go to";
const string fleet_action::join = "join";
const string fleet_action::follow = "follow";
const string fleet_action::idle = "idle";

set<string> fleet::all_interactions(){
  return {fleet_action::space_combat, fleet_action::bombard, fleet_action::colonize};
}

set<string> fleet::all_base_actions(){
  return {fleet_action::go_to, fleet_action::join, fleet_action::follow, fleet_action::idle};
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
    data[fleet_action::go_to] = target_condition(target_condition::owned, target_condition::no_target);
    data[fleet_action::join] = target_condition(target_condition::owned, fleet::class_id);
    data[fleet_action::follow] = target_condition(target_condition::any_alignment, fleet::class_id);
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
  check_join(g);
}

void fleet::move(game_data *g){
  // linear extrapolation of position estimate
  position += speed_limit * utility::normv(utility::point_angle(target_position - position));
}

void fleet::interact(game_data *g){}

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

  // check target status valid
  target_condition c = fleet::action_condition_table()[com.action];
  if (c.requires_target() && !interaction::macro_valid(c.owned_by(owner), g -> get_entity(com.target))){
    cout << "target " << com.target << " no longer valid for " << id << endl;
    com.target = identifier::target_idle;
    com.action = fleet_action::idle;
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

void fleet::check_join(game_data *g){
  // check fleet joins
  if (com.action == fleet_action::join && converge){

    // check that the fleet exists
    if (!g -> entity.count(com.target)){
      cout << "fleet " << id << ": target " << com.target << " missing, setting idle:0" << endl;
      com.target = identifier::target_idle;
      com.action = fleet_action::idle;
      return;
    }

    fleet::ptr f = g -> get_fleet(com.target);

    for (auto i : ships){
      ship::ptr s = g -> get_ship(i);
      s -> fleet_id = com.target;
      f -> ships.insert(i);
    }
    remove = true;
  }
}

void fleet::check_in_sight(game_data *g){
  if (is_idle()) return;

  bool seen = g -> entity_seen_by(com.target, owner);
  bool solar = identifier::get_type(com.target) == solar::class_id;

  // target left sight?
  if (!(seen || solar)){
    cout << "fleet " << id << " looses sight of " << com.target << endl;

    // // create a waypoint and reset target
    // waypoint::ptr w = waypoint::create(owner);

    // // if the target exists, set waypoint at  target position
    // g -> target_position(com.target, w -> position);
    // g -> add_entity(w);

    com.target = identifier::target_idle;
    com.action = fleet_action::idle;
  }
}

bool fleet::confirm_ship_interaction(string a){
  return com.action == a;
}

void fleet::copy_from(const fleet &s){
  (*this) = s;
}

void fleet::remove_ship(combid i){
  ships.erase(i);
  remove = ships.empty();
}
