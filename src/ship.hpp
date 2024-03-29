#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "cost.hpp"
#include "game_object.hpp"
#include "ship_stats.hpp"
#include "types.hpp"

namespace st3 {
class game_data;

/*! ship game object */
class ship : public virtual physical_object, public ship_stats, public std::enable_shared_from_this<ship> {
 public:
  typedef ship_ptr ptr;
  static ship_ptr create();
  static const std::string class_id;
  static std::vector<std::string> all_classes();
  static std::string starting_ship;
  static const int na;
  static const float friction;

  idtype fleet_id; /*!< id of the ship's fleet */
  sfloat angle;    /*!< ship's angle */
  sfloat thrust;
  point force;
  point velocity;
  sfloat load;
  ssfloat_t base_stats;
  int nkills;
  cost::res_t cargo;

  // state trackers for specific ship types
  std::set<std::string> states;

  // ai stats
  float target_angle;
  float target_speed;
  bool activate;
  bool skip_head;
  bool force_refresh;
  float hpos;
  idtype current_target;
  float collision_damage;
  path_t private_path;
  point pp_backref;
  std::string pathing_policy;

  std::list<idtype> neighbours;
  std::list<idtype> local_enemies;
  std::list<idtype> local_friends;
  std::list<idtype> local_all;
  bool fleet_center_blocked;

  // game_object
  void pre_phase(game_data *g);
  void move(game_data *g);
  void post_phase(game_data *g);
  void on_remove(game_data *g);
  float vision();
  bool serialize(sf::Packet &p);
  bool is_active();
  game_object_ptr clone();
  bool isa(std::string c);

  // physical_object
  std::set<std::string> compile_interactions();
  float interaction_radius();
  bool can_see(game_object_ptr x);

  // ship
  ship(const ship &s);
  ship(const ship_stats &s);
  ship() = default;
  ~ship() = default;

  float speed();
  float max_speed();
  float flex_weight(float a);
  void update_data(game_data *g);
  void receive_damage(game_data *g, game_object_ptr from, float damage);
  void on_liftoff(solar_ptr from, game_data *g);
  bool has_fleet();
  float evasion_check();
  float accuracy_check(ship_ptr a);

 protected:
  // Pathing alternatives
  bool follow_fleet_heading(game_data *g);
  bool follow_fleet_trail(game_data *g);
  bool follow_private_path(game_data *g);
  bool build_private_path(game_data *g, point p);
};
};  // namespace st3
