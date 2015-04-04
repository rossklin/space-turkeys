#ifndef _STK_CLIENTGAME
#define _STK_CLIENTGAME

#include <set>
#include <SFML/Graphics.hpp>

#include "socket_t.h"
#include "graphics.h"
#include "game_data.h"
#include "choice.h"
#include "types.h"

namespace st3{
  namespace client{
    struct game{
      socket_t socket;
      window_t window;
      game_data data;
      choice genchoice;

      // choice interface data
      std::set<idtype> selected_solars;

      void run();
      bool pre_step();
      void choice_step();
      void simulation_step();
      void choice_event(sf::Event e);
      void area_select(sf::FloatRect r);

      // graphics
      void draw_universe();
    };
  };
};
#endif
