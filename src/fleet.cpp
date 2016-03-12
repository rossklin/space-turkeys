#include <iostream>

#include "fleet.h"
#include "ship.h"
#include "utility.h"
#include "game_data.h"

using namespace std;
using namespace st3;

fleet::fleet(){
  position = point(0,0);
  radius = 0;
  update_counter = 0;
  owner = -1;
  converge = false;
  speed_limit = 0;
  remove = false;
}

void fleet::pre_phase(game_data *g){
  update_data(g);
  check_in_sight(g);
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
  return dynamic_pointer_cast<fleet>(clone_impl());
}

game_object::ptr fleet::clone_impl(){
  return ptr(new fleet(*this));
}

bool st3::fleet::is_idle(){
  return !identifier::get_type(com.target).compare(identifier::idle);
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
	  ships.erase(i);
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
  
  radius = fmax(sqrt(r2), fleet::min_radius);
  speed_limit = speed;  
  position = utility::scale_point(p, 1 / (float)ships.size());

  // check target status valid
  if (!interaction::valid(fleet::action_condition_table(com.action, owner), g -> entity[com.target])){
    cout << "target " << com.target << " no longer valid for " << id << endl;
    com.target = identifier::target_idle;
  }

  // have arrived?
  if (!is_idle()){
    if (g -> target_position(com.target, target_position)) converge = utility::l2d2(target_position - position) < fleet::interact_d2;
  }
}

void fleet::check_waypoint(game_data *g){
  // set to idle and 'land' ships if converged to waypoint
  if (converge && identifier::get_type(com.target) == identifier::waypoint && com.action == command::action_waypoint){
    com.target = identifier::make(identifier::idle, com.target);
    cout << "set fleet " << id << " idle target: " << com.target << endl;
  }
}

void fleet::check_join(game_data *g){
  // check fleet joins
  if (com.action == command::action_join && converge){

    // check that the fleet exists
    if (!g -> entity.count(com.target)){
      cout << "fleet " << id << ": target " << com.target << " missing, setting idle:0" << endl;
      com.target = identifier::target_idle;
      return;
    }

    fleet::ptr f = g -> get_fleet(com.target);

    for (auto i : ships){
      ship::ptr s = g -> get_ship(i);
      s -> fleet_id = com.target;
      f -> ships.insert(i);
      remove = true;
    }
  }
}

void fleet::check_in_sight(game_data *g){
  // target left sight?
  if ((!is_idle()) && (!g -> entity_seen_by(com.target, owner))){

    cout << "fleet " << id << " looses sight of " << com.target << endl;
    // create a waypoint and reset target
    waypoint::ptr w = waypoint::create(owner);

    if (!g -> target_position(com.target, w -> position)){
      cout << "unset fleet target: target position not found: " << com.target << endl;
      exit(-1);
    }

    com.target = w -> id;
    com.action = command::action_waypoint;
    g -> add_entity(w);
  }
}
