#pragma once

#include "game_object.h"
#include "game_settings.h"
#include "player.h"
#include "types.h"
#include "utility.h"

namespace st3 {
class game_base_data {
 public:
  hm_t<class_t, idtype> idc;
  hm_t<idtype, player> players; /*!< table of players */
  game_settings settings;       /*! game settings */
  hm_t<combid, game_object::ptr> entity;
  hm_t<idtype, terrain_object> terrain;
  std::list<combid> remove_entities;
  hm_t<idtype, std::set<combid> > evm;
  std::set<std::pair<int, int> > discovered_universe;

  virtual ~game_base_data() = default;
  int next_id(class_t x);
  void clear_entities();

  // access
  ship::ptr get_ship(combid i) const;
  fleet::ptr get_fleet(combid i) const;
  solar::ptr get_solar(combid i) const;
  waypoint::ptr get_waypoint(combid i) const;
  game_object::ptr get_entity(combid i) const;

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

  void limit_to(idtype pid);
  void copy_from(const game_data &g);
  int get_max_fleets(idtype pid) const;
  int get_max_ships_per_fleet(idtype pid) const;
};
};  // namespace st3