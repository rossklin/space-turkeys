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
      bool area_select_active;

      idtype comid;
      sf::FloatRect srect;
      hm_t<idtype, command> commands;
      hm_t<source_t, std::set<source_t> > entity_commands;
      hm_t<source_t, entity_selector*> entity_selectors;

      // round sections
      void run();
      bool pre_step();
      void choice_step();
      void simulation_step();

      // data handling
      choice build_choice();
      void initialize_selectors();

      // event handling
      bool choice_event(sf::Event e);
      void area_select();
      source_t entity_at(point p);
      void target_at(point p);
      void select_at(point p);

      // command handling
      bool command_exists(command c);
      void command_points(command c, point &from, point &to);
      void add_command(command c, point from, point to);
      void remove_command(source_t key);
      void command2point(point p);
      void command2entity(source_t key);

      // selection handling
      void clear_selectors();
      void deselect_all();

      // graphics
      void draw_universe(game_data &g);
      void draw_universe_ui();
    };
  };
};
#endif
