#ifndef _STK_GAMESETTINGS
#define _STK_GAMESETTINGS

#include "types.h"

namespace st3{
  /*! Struct containing settings for a game */
  struct game_settings{
    sint frames_per_round; /*!< number of frames in the simulation step */
    sint width; /*!< width of the coordinate system */
    sint height; /*!< height of the coordinate system */
    sfloat ship_speed; /*!< base ship speed (coordinate length per unit time) */
    sfloat solar_minrad; /*!< smallest solar radius */
    sfloat solar_maxrad; /*!< largest solar radius */
    sint num_solars; /*!< number of solars */
    sfloat fleet_default_radius; /*!< default radius for fleets */
    sfloat dt; /*!< game time per iteration step */

    /*! default settings */
    game_settings();
  };
};
#endif

