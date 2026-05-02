#pragma once

#include <rapidjson/document.h>

#include <list>
#include <memory>
#include <set>
#include <vector>

#include "choice.hpp"
#include "cost.hpp"
#include "game_object.hpp"
#include "types.hpp"

namespace st3 {
class game_data;

/*! data representing a solar system */
class solar : public virtual physical_object, public virtual commandable_object, public std::enable_shared_from_this<solar> {
 public:
  typedef solar_ptr ptr;
  static solar_ptr create(idtype id, point p, float bounty, float var = 0.3);
  static const std::string class_id;

  c_solar choice_data;

  sfloat ship_progress;
  sfloat hp;

  sbool was_discovered;
  std::set<idtype> known_by;

  std::set<idtype> ships;

  solar() = default;
  ~solar() = default;
  solar(const solar &s);

  // game_object
  void pre_phase(game_data *g);
  void move(game_data *g);
  void post_phase(game_data *g);
  bool serialize(sf::Packet &p);
  game_object_ptr clone();
  bool isa(std::string c);

  // physical_object
  std::set<std::string> compile_interactions();
  float interaction_radius();
  bool can_see(game_object_ptr x);

  // commandable_object
  void give_commands(std::list<command> c, game_data *g);

  // solar
  void receive_damage(game_object_ptr s, float damage, game_data *g);
  float vision();
  std::string get_info();
  void dynamics(game_data *g);
  float max_hp();

 protected:
};
};  // namespace st3
