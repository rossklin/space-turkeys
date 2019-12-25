#pragma once

#include <set>
#include <string>
#include "types.h"

namespace st3 {
/*! Struct containing settings for a game */
extern const std::set<std::string> starting_options;
extern int sub_frames;

struct client_game_settings {
  std::string starting_fleet;
  sint num_players;
  sint frames_per_round; /*!< number of frames in the simulation step */
  sfloat galaxy_radius;
  sint restart;

  client_game_settings();
  bool validate();
};

struct game_settings : public client_game_settings {
  sfloat solar_minrad;
  sfloat solar_meanrad;
  sfloat solar_density;        /*!< solars per space unit */
  sfloat fleet_default_radius; /*!< default radius for fleets */
  sfloat dt;                   /*!< game time per iteration step */
  int space_index_ratio;
  bool enable_extend;

  game_settings();
};
};  // namespace st3
