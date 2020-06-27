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
  void clear_entities();
  void copy_from(const game_base_data &g);
  int get_max_fleets(idtype pid) const;
  int get_max_ships_per_fleet(idtype pid) const;

  bool entity_exists(combid id) const;
  void add_entity(game_object::ptr e);
  void remove_entity(combid id);

  template <typename T>
  std::vector<typename T::ptr> all_entities(idtype pid = game_object::any_owner) const {
    std::vector<T::ptr> res;
    for (auto p : entity) res.push_back(utility::guaranteed_cast<T>(p.second));
    return res;
  };

  template <typename T>
  std::vector<typename T::ptr> filtered_entities(idtype pid = game_object::any_owner) const {
    std::vector<T::ptr> res;

    for (auto p : entity) {
      if (p.second->isa(T::class_id) && (pid == game_object::any_owner || p.second->owner == pid)) {
        res.push_back(utility::guaranteed_cast<T>(p.second));
      }
    }

    return res;
  };

  template <typename T>
  typename T::ptr get_entity(combid i) const {
    if (entity.count(i)) {
      return utility::guaranteed_cast<T>(entity.at(i));
    } else {
      throw logical_error("get_entity: not found!");
    }
  }
};
};  // namespace st3