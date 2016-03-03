#ifndef _STK_GAMEHANDLER
#define _STK_GAMEHANDLER

#include "game_data.h"
#include "com_server.h"

namespace st3{
  namespace server{
    /*! run a game with given clients and game data
      @param c com object with client data
      @param g game data
    */
    void game_handler(com &c, game_data &g);
  };
};

#endif
