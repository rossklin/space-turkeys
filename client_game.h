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
#include "utility.h"

namespace st3{
  namespace client{
    struct game{
      socket_t socket;
      window_t window;
      bool area_select_active;
      sf::FloatRect srect;
      idtype comid;
      game_settings settings;

      hm_t<idtype, player> players;
      hm_t<idtype, ship> ships;
      hm_t<source_t, std::set<source_t> > entity_commands;
      hm_t<source_t, entity_selector*> entity_selectors;
      hm_t<source_t, command_selector*> command_selectors;

      // round sections
      void run();
      bool pre_step();
      void choice_step();
      void simulation_step();

      // data handling
      choice build_choice();
      void initialize_selectors();
      void reload_data(const game_data &g);

      // event handling
      bool choice_event(sf::Event e);
      void area_select();
      source_t entity_at(point p);
      void target_at(point p);
      void select_at(point p);
      void controls();

      // command handling
      bool command_exists(command c);
      void command_points(command c, point &from, point &to);
      void add_command(command c, point from, point to);
      void remove_command(source_t key);
      void command2point(point p);
      void command2entity(source_t key);
      void increment_command(source_t key, int delta);

      // selection handling
      void clear_selectors();
      void deselect_all();

      // graphics
      void draw_universe();
    };
  };
};
#endif
