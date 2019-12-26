#pragma once

#include <SFML/Network.hpp>

#include "animation_data.h"
#include "choice.h"
#include "fleet.h"
#include "game_object.h"
#include "game_settings.h"
#include "grid_tree.h"
#include "interaction.h"
#include "player.h"
#include "ship.h"
#include "solar.h"
#include "types.h"
#include "utility.h"
#include "waypoint.h"

namespace st3 {
namespace server {
struct client_t;
};

struct interaction_info {
  combid source;
  combid target;
  std::string interaction;
};

class entity_package {
 public:
  hm_t<class_t, idtype> idc;
  hm_t<idtype, player> players; /*!< table of players */
  game_settings settings;       /*! game settings */
  hm_t<combid, game_object::ptr> entity;
  hm_t<idtype, terrain_object> terrain;
  std::list<combid> remove_entities;
  hm_t<idtype, std::set<combid> > evm;
  std::set<std::pair<int, int> > discovered_universe;

  virtual ~entity_package() = default;
  int next_id(class_t x);
  void clear_entities();

  // access
  ship::ptr get_ship(combid i) const;
  fleet::ptr get_fleet(combid i) const;
  solar::ptr get_solar(combid i) const;
  waypoint::ptr get_waypoint(combid i) const;
  game_object::ptr get_entity(combid i) const;

  void limit_to(idtype pid);
  void copy_from(const game_data &g);
  std::list<game_object::ptr> all_owned_by(idtype pid) const;
};

/*! struct containing data for game objects */
class game_data : public virtual entity_package {
 public:
  static void confirm_data();

  grid::tree::ptr entity_grid;
  std::vector<interaction_info> interaction_buffer;

  game_data();
  ~game_data();
  game_data(const game_data &g) = delete;
  void assign(const game_data &g);
  void rehash_grid();
  void apply_choice(choice::choice c, idtype id);
  void increment();
  bool target_position(combid t, point &p) const;
  std::list<combid> search_targets(combid self_id, point p, float r, target_condition c) const;
  std::list<combid> search_targets_nophys(combid self_id, point p, float r, target_condition c) const;
  int first_intersect(point a, point b, float r) const;
  path_t get_path(point a, point b, float r) const;
  void rebuild_evm();
  idtype terrain_at(point p, float r) const;
  void extend_universe(int i, int j, bool starting_area = false);
  void discover(point x, float r, bool starting_area = false);
  void update_discover();
  solar::ptr closest_solar(point p, idtype id) const;
  animation_tracker_info get_tracker(combid id) const;
  void log_ship_fire(combid a, combid b);
  void log_ship_destroyed(combid a, combid b);
  void log_bombard(combid a, combid b);
  void log_message(combid a, std::string v_full, std::string v_short);
  float get_dt() const;
  bool allow_add_fleet(idtype pid) const;

  template <typename T>
  std::list<typename T::ptr> all(idtype pid = game_object::any_owner) const {
    std::list<typename T::ptr> res;

    for (auto p : entity) {
      if (p.second->isa(T::class_id) && (pid == game_object::any_owner || p.second->owner == pid)) {
        res.push_back(utility::guaranteed_cast<T>(p.second));
      }
    }

    return res;
  };

  void add_entity(game_object::ptr p);
  void remove_units();
  fleet::ptr generate_fleet(point p, idtype i, command c, std::list<combid> sh, bool ignore_limit = false);
  void relocate_ships(command c, std::set<combid> &sh, idtype owner);
  void allocate_grid();

  // game steps
  void pre_step();
  void end_step();
  void build_players(std::list<server::client_t *> clients);
  void build();

 protected:
  // object iteration phases
  void pre_phase();
  void update();
  void post_phase();

  // add and remove entities
  void remove_entity(combid id);
  void distribute_ships(fleet::ptr f);
  void update_research_facility_level();
};
};  // namespace st3
