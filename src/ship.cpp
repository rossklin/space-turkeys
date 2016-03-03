#include "ship.h"

using namespace std;
using namespace st3;

void ship::pre_phase(game_data *g){
  // load weapons
  load = fmin(load + 1, load_time);
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
  
  // check land
  if (f -> com.action == command::action_land && f -> converge){
    solar::ptr s = g -> get_solar(f -> com.target);
    if (utility::l2d2(s -> position - position) < pow(s -> radius, 2)){
      s -> ships.push_back(make_shared(this));
      remove = true;
    }
  }

  // check registered interactions
  auto inter = compile_interactions();
  for (auto x : inter){
    list<combid> valid_targets = g -> search_targets(position, x.radius, x.target_condition);
    for (auto a : valid_targets){
      x.perform(make_shared(this), g -> entity[a]);
    }
  }
}

void ship::post_phase(game_data *g){}

void ship::on_remove(game_data *g){
  g -> get_fleet(fleet_id) -> ships.erase(id);
  game_object::on_remove(g);
}
