#ifndef _STK_GAMEDATA
#define _STK_GAMEDATA

#include <SFML/Network.hpp>

#include "types.h"
#include "ship.h"
#include "solar.h"
#include "choice.h"
#include "player.h"

namespace st3{
  struct game_settings{
    sint frames_per_round;
  };

  struct game_data{
    hm_t<idtype, ship> ships;
    hm_t<idtype, solar> solars;
    hm_t<idtype, player> players;
    game_settings settings;
  
    void apply_choice(choice c, sint id);
    void increment();
    void build();
    void cleanup();
  };
};
#endif
