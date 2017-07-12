#ifndef _STK_GAMESETTINGS
#define _STK_GAMESETTINGS

#include <string>
#include <set>
#include "types.h"

namespace st3{
  /*! Struct containing settings for a game */
  extern const std::set<std::string> starting_options;
  struct game_settings{
    
    std::string starting_fleet;
    sint num_players;
    sint frames_per_round; /*!< number of frames in the simulation step */
    sfloat galaxy_radius;
    sfloat solar_minrad;
    sfloat solar_meanrad;
    sfloat solar_density; /*!< solars per space unit */
    sfloat fleet_default_radius; /*!< default radius for fleets */
    sfloat dt; /*!< game time per iteration step */

    /*! default settings */
    game_settings();
    bool validate();
  };
};
#endif

