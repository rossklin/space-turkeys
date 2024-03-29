#pragma once

#include <functional>

#include "types.hpp"

namespace st3 {
class game_data;

class target_condition {
 public:
  static const sint neutral = 1;
  static const sint owned = 1 << 1;
  static constexpr sint enemy = 1 << 2;
  static constexpr sint any_alignment = -1;
  static const class_t no_target;

  class_t what;
  sint alignment;
  idtype owner;

  target_condition();
  target_condition(sint a, class_t w);
  target_condition owned_by(idtype o);
  bool requires_target();
  bool valid_on(game_object_ptr o);

  static bool get_alignment(idtype t, idtype s);
};

class interaction {
 public:
  static const std::string trade_to;
  static const std::string trade_from;
  static const std::string land;
  static const std::string deploy;
  static const std::string search;
  static const std::string auto_search;
  static const std::string turret_combat;
  static const std::string space_combat;
  static const std::string bombard;
  static const std::string colonize;
  static const std::string pickup;
  static const std::string terraform;
  static const std::string hive_support;
  static const std::string splash;

  typedef std::function<void(game_object_ptr self, game_object_ptr target, game_data *g)> perform_t;
  static const hm_t<std::string, interaction> &table();

  std::string name;
  target_condition condition;
  perform_t perform;
};
};  // namespace st3
