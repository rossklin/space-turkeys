#include "fleet.h"

st3::fleet::fleet(){
  position = point(0,0);
  radius = 0;
  update_counter = 0;
  owner = -1;
  converge = false;
  speed_limit = 0;
  remove = false;
}

bool st3::fleet::is_idle(){
  return !identifier::get_type(com.target).compare(identifier::idle);
}

void fleet::update_data(game_data *g){

  // need to update fleet data?
  if (((update_counter++) % fleet::update_period)) return;
  float speed = INFINITY;
  int count;

  // position, radius, speed and vision
  point p(0,0);
  float r2 = 0;
  vision = 0;

  for (auto k : ships){
    ship::ptr s = g -> get_ship(k);
    speed = fmin(speed, s.speed);
    r2 = fmax(r2, utility::l2d2(s.position - position));
    vision = fmax(vision, s.vision);
    p = p + s.position;
  }
  
  radius = fmax(sqrt(r2), fleet::min_radius);
  speed_limit = speed;  
  position = utility::scale_point(p, 1 / (float)ships.size());
}

void fleet::check_converge(game_data *g){
  // have arrived?
  if (!is_idle()){
    point target;
    if (g -> target_position(com.target, target)) converge = utility::l2d2(target - position) < fleet::interact_d2;
  }
}

void fleet::check_waypoint(){
  // set to idle and 'land' ships if converged to waypoint
  if (converge && identifier::get_type(com.target) == identifier::waypoint && com.action == command::action_waypoint){
    com.target = identifier::make(identifier::idle, com.target);
    cout << "set fleet " << fid << " idle target: " << com.target << endl;
  }
}

void fleet::check_join(game_data *g){
  // check fleet joins
  if (com.action == command::action_join && converge){

    // check that the fleet exists
    if (!g -> entity.count(com.target)){
      cout << "fleet " << fid << ": target " << com.target << " missing, setting idle:0" << endl;
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

    cout << "fleet " << fid << " looses sight of " << com.target << endl;
    // create a waypoint and reset target
    waypoint::ptr w = waypoint::create(owner);

    if (!target_position(com.target, w -> position)){
      cout << "unset fleet target: target position not found: " << com.target << endl;
      exit(-1);
    }

    com.target = w -> id;
    com.action = command::action_waypoint;
    add_entity(w);
  }
}
