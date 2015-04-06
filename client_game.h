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

      struct selector_t{
	idtype id;
	sint type;
	bool owned;

	selector_t(idtype i, sint t, bool o);
	bool operator < (selector_t b);
      };

      std::set<selector_t> selectors;

      const sint SELECT_NONE = 0; 
      const sint SELECT_SOLAR = 1;
      const sint SELECT_FLEET = 2;
      const sint SELECT_SOLAR_COMMAND = 4;
      const sint SELECT_FLEET_COMMAND = 8;

      void run();
      bool pre_step();
      void choice_step();
      void simulation_step();
      void choice_event(sf::Event e);
      void area_select(sf::FloatRect r);
      void click_at(point p, sf::Mouse::Button button);

      // graphics
      void draw_universe();
    };
  };
};
#endif
