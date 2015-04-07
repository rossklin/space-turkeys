#ifndef _STK_CLIENTGAME
#define _STK_CLIENTGAME

#include <set>
#include <SFML/Graphics.hpp>

#include "socket_t.h"
#include "graphics.h"
#include "game_data.h"
#include "choice.h"
#include "types.h"
#include "selector.h"

namespace st3{
  namespace client{
    struct game{
      socket_t socket;
      window_t window;
      game_data data;

      idtype comid;
      sf::FloatRect srect;
      hm_t<idtype, command_selector*> command_selectors;
      hm_t<source_t, std::set<idtype>, source_t::hash, source_t::pred> entity_commands;
      hm_t<source_t, entity_selector*, source_t::hash, source_t::pred> entity_selectors;

      void run();
      bool pre_step();
      void choice_step();
      void simulation_step();

      choice build_choice();
      void initialize_selectors();

      bool choice_event(sf::Event e);
      void area_select();
      void click_at(point p, sf::Mouse::Button button);

      bool command_exists(command c);
      void command_points(command c, point &from, point &to);
      void add_command(command c);
      void remove_command(idtype id);
      void command2point(point p);
      void command2entity(entity_selector *s);

      void clear_selectors();
      void deselect_all();

      // graphics
      void draw_universe(game_data &g);
      void draw_universe_ui();
    };
  };
};
#endif
