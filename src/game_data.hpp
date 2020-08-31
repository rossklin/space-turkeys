#pragma once

#include <SFML/Network.hpp>
#include <vector>

#include "animation_data.hpp"
#include "choice.hpp"
#include "game_base_data.hpp"
#include "interaction.hpp"
#include "types.hpp"

namespace st3 {

struct interaction_info {
  combid source;
  combid target;
  std::string interaction;
};

/*! struct containing data for game objects */
class game_data : public virtual game_base_data {
 public:
  static void confirm_data();

  std::vector<interaction_info> interaction_buffer;

  game_data() = default;
  ~game_data() = default;
  game_data(const game_data &g) = delete;

  int next_id(class_t x);

  // access
  ship_ptr get_ship(combid i) const;
  fleet_ptr get_fleet(combid i) const;
  solar_ptr get_solar(combid i) const;
  waypoint_ptr get_waypoint(combid i) const;

  game_object_ptr get_game_object(combid i) const;

  void assign(const game_data &g);
  void apply_choice(choice c, idtype id);
  void increment(bool test_extend = true);
  bool target_position(combid t, point &p) const;
  std::list<combid> search_targets(combid self_id, point p, float r, target_condition c, int knn = 0) const;
  std::list<combid> search_targets_nophys(combid self_id, point p, float r, target_condition c, int knn = 0) const;
  int first_intersect(point a, point b, float r) const;
  path_t get_path(point a, point b, float r) const;
  void rebuild_evm();
  idtype terrain_at(point p, float r) const;
  void extend_universe(int i, int j, bool starting_area = false);
  void discover(point x, float r, bool starting_area = false);
  void update_discover();
  solar_ptr closest_solar(point p, idtype id) const;
  animation_tracker_info get_tracker(combid id) const;
  void log_ship_fire(combid a, combid b);
  void log_ship_destroyed(combid a, combid b);
  void log_bombard(combid a, combid b);
  void log_message(combid a, std::string v_full, std::string v_short);
  float get_dt() const;
  bool allow_add_fleet(idtype pid) const;
  float solar_order_level(combid id) const;

  void register_entity(game_object_ptr p);
  void remove_units();
  fleet_ptr generate_fleet(point p, idtype i, command c, std::list<combid> sh, bool ignore_limit = false);
  void relocate_ships(command c, std::set<combid> &sh, idtype owner);

  // game steps
  void pre_step();
  void end_step();
  void build_players(std::vector<server_cl_socket_ptr> clients);
  void build();

 protected:
  // object iteration phases
  void pre_phase();
  void update();
  void post_phase();

  // add and remove entities
  void deregister_entity(combid id);
  void distribute_ships(fleet_ptr f);
  void update_research_facility_level();
};
};  // namespace st3
