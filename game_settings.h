#ifndef _STK_GAMESETTINGS
#define _STK_GAMESETTINGS

#include "types.h"

namespace st3{
  struct game_settings{
    sint frames_per_round;
    sint width;
    sint height;
    sfloat ship_speed;
    sfloat solar_minrad;
    sfloat solar_maxrad;
    sint num_solars;
    sfloat fleet_default_radius;

    game_settings();
  };
};
#endif

