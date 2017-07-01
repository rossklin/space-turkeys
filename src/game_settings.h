#ifndef _STK_GAMESETTINGS
#define _STK_GAMESETTINGS

#include "types.h"

namespace st3{
  /*! Struct containing settings for a game */
  struct game_settings{
    sint frames_per_round; /*!< number of frames in the simulation step */
    sfloat galaxy_radius;
    sfloat ship_speed; /*!< base ship speed (coordinate length per unit time) */
    sfloat solar_minrad;
    sfloat solar_meanrad;
    sfloat solar_density; /*!< solars per space unit */
    sfloat fleet_default_radius; /*!< default radius for fleets */
    sfloat dt; /*!< game time per iteration step */

    /*! default settings */
    game_settings();
  };
};
#endif

