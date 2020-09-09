#pragma once

#include <set>

#include "command.hpp"
#include "game_object.hpp"
#include "ship_stats.hpp"
#include "types.hpp"

namespace st3 {
class game_data;

namespace fleet_action {
extern const std::string go_to;
extern const std::string idle;
};  // namespace fleet_action

/*! a fleet controls a set of ships */
class fleet : public virtual commandable_object {
 public:
  typedef fleet_ptr ptr;
  static fleet_ptr create(idtype pid, idtype idx);
  static const std::string class_id;

  struct analytics {
    std::list<std::pair<point, float> > enemies;
    ssfloat_t average_ship;
    sbool converge;
    sfloat vision_buf;
    float speed_limit;
    point target_position;
    float spread_radius;
    float spread_density;
    point evade_path;
    sbool can_evade;
    float facing_ratio;

    analytics();
  };

  static const idtype server_pid;
  static const int update_period; /*!< number of increments between fleet data updates */
  static const int interact_d2;   /*!< distance from target at which the fleet converges */

  // fleet policies
  static const sint policy_aggressive;
  static const sint policy_evasive;
  static const sint policy_maintain_course;

  static sint default_policy(std::string action);

  // serialized components
  std::set<combid> ships;             /*!< ids of ships in fleet */
  std::set<std::string> interactions; /*!< set of available interactions */
  command com;                        /*!< the fleet's command (currently this only holds the target) */
  point heading;
  int heading_index;
  std::vector<point> path;

  // mechanical components
  int update_counter; /*!< counter for updating fleet data */
  analytics stats;
  bool force_refresh;
  std::set<combid> helper_fleets;

  // game_object stuff
  void pre_phase(game_data *g) override;
  void move(game_data *g) override;
  void post_phase(game_data *g) override;
  float vision() override;
  bool serialize(sf::Packet &p) override;
  bool isa(std::string c) override;
  void on_remove(game_data *g) override;
  game_object_ptr clone() override;
  bool is_active() override;

  // commandable object stuff
  void give_commands(std::list<command> c, game_data *g) override;

  // fleet stuff
  fleet(idtype pid, idtype idx);
  fleet() = default;
  ~fleet() = default;
  fleet(const fleet &f);

  bool is_idle();
  void set_idle();
  void analyze_enemies(game_data *g);
  void update_data(game_data *g, bool set_force_refresh = false);
  void remove_ship(combid i);
  float get_hp();
  float get_dps();
  float get_strength();
  void refresh_ships(game_data *g);
  void check_action(game_data *g);
  // combid request_helper_fleet(game_data *g, combid sid);
  // void gather_helper_fleets(game_data *g);

 protected:
  void check_waypoint(game_data *g);
  void check_in_sight(game_data *g);
  void build_path(game_data *g, point t);
  void update_heading(game_data *g);
};
};  // namespace st3
