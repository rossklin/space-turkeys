#ifndef _STK_GAMEDATA
#define _STK_GAMEDATA

#include <SFML/Network.hpp>

#include "types.h"
#include "ship.h"
#include "fleet.h"
#include "solar.h"
#include "choice.h"
#include "player.h"

namespace st3{
  struct game_settings{
    sint frames_per_round;
    sint width;
    sint height;
    sfloat ship_speed;
    sfloat solar_minrad;
    sfloat solar_maxrad;
    sint num_solars;

    game_settings();
  };

  struct game_data{
    hm_t<idtype, ship> ships;
    hm_t<idtype, fleet> fleets;
    hm_t<idtype, solar> solars;
    hm_t<idtype, player> players;
    game_settings settings;

    // server mechanics
    void apply_choice(choice c, sint id);
    void increment();
    void cleanup(); // remove dead ships

    // build stuff
    void build();
    hm_t<idtype, solar> random_solars();
    float heuristic_homes(hm_t<idtype, solar> solar_buf, hm_t<idtype, idtype> &start_solars);
  };
};
#endif
