#pragma once

#include "game_base_data.h"
#include "game_settings.h"
#include "player.h"
#include "selector.h"
#include "types.h"

namespace st3 {
struct client_game_data : public game_base_data {
  hm_t<combid, client::entity_selector::ptr> cl_entity; /*!< graphical representations for solars, fleets and waypoints */
};
};  // namespace st3
