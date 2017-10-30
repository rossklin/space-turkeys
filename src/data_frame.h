#ifndef _STK_DATA_FRAME
#define _STK_DATA_FRAME

#include "selector.h"
#include "game_settings.h"
#include "player.h"
#include "types.h"

namespace st3{
  struct data_frame{
    game_settings settings; /*!< the game settings */
    hm_t<idtype, player> players; /*!< data for players in the game */
    hm_t<combid, client::entity_selector::ptr> entity; /*!< graphical representations for solars, fleets and waypoints */
    std::list<combid> remove_entities;
    hm_t<idtype, terrain_object> terrain;
  };
};

#endif
