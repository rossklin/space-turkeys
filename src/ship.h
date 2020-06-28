#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "cost.h"
#include "game_object.h"
#include "interaction.h"
#include "ship_stats.h"
#include "solar.h"
#include "types.h"

namespace st3 {
class game_data;
class solar;

/*! ship game object */
class ship : public virtual physical_object, public ship_stats, public std::enable_shared_from_this<ship> {
 public:
  typedef std::shared_ptr<ship> ptr;
  static ptr create();
  static const std::string class_id;
  static std::vector<std::string> all_classes();
  static std::string starting_ship;
  static const int na;
  static const float friction;

  combid fleet_id; /*!< id of the ship's fleet */
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
  path_t private_path;
  float hpos;
  combid current_target;
  float collision_damage;

  std::list<combid> neighbours;
  std::list<combid> local_enemies;
  std::list<combid> local_friends;
  std::list<combid> local_all;
  std::vector<float> free_angle;
  // bool check_space(float a);

  // game_object
  void pre_phase(game_data *g);
  void move(game_data *g);
  void post_phase(game_data *g);
  void on_remove(game_data *g);
  float vision();
  bool serialize(sf::Packet &p);
  bool is_active();
  game_object::ptr clone();
  bool isa(std::string c);

  // physical_object
  std::set<std::string> compile_interactions();
  float interaction_radius();
  bool can_see(game_object::ptr x);

  // ship
  ship(const ship &s);
  ship(const ship_stats &s);
  ship() = default;
  ~ship() = default;

  float speed();
  float max_speed();
  float flex_weight(float a);
  void update_data(game_data *g);
  void receive_damage(game_data *g, game_object::ptr from, float damage);
  void on_liftoff(solar::ptr from, game_data *g);
  bool has_fleet();
  float evasion_check();
  float accuracy_check(ship::ptr a);
};
};  // namespace st3
