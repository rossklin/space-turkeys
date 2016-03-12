#ifndef _STK_BUILD_UNIVERSE
#define _STK_BUILD_UNIVERSE

#include "types.h"
#include "solar.h"
#include "game_settings.h"

namespace st3{
  namespace build_universe{
    hm_t<combid, solar> random_solars(game_settings settings);
    float heuristic_homes(hm_t<combid, solar> solar_buf, hm_t<idtype, combid> &start_solars, game_settings settings, std::vector<idtype> player_ids);
  };
};

#endif
