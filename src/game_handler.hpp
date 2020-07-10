#pragma once

#include "com_server.hpp"
#include "game_data.hpp"

namespace st3 {
namespace server {
/*! run a game with given clients and game data
      @param c com object with client data
      @param g game data
    */
void game_handler(game_setup c, game_data &g);
};  // namespace server
};  // namespace st3
