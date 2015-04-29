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
#include "command_gui.h"

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
      hm_t<source_t, entity_selector*> entity_selectors;
      hm_t<idtype, command_selector*> command_selectors;

      command_gui *comgui;

      game();

      // round sections
      void run();
      bool pre_step();
      void choice_step();
      void simulation_step();

      // data handling
      command build_command(idtype key);
      choice build_choice();
      void initialize_selectors();
      void reload_data(game_data &g);

      // event handling
      int choice_event(sf::Event e);
      void area_select();
      source_t entity_at(point p);
      idtype command_at(point p);
      void target_at(point p);
      bool select_at(point p);
      bool select_command_at(point p);
      void controls();
      void clear_guis();
      void recursive_waypoint_deallocate(source_t wid, std::set<idtype> a);

      // command handling
      bool command_exists(command c);
      idtype command_id(command c);
      void command_points(command c, point &from, point &to);
      void add_command(command c, point from, point to, bool fill_ships = true);
      void remove_command(idtype key);
      void command2entity(source_t key);
      source_t add_waypoint(point p);

      // selection handling
      void clear_selectors();
      void deselect_all();
      std::list<idtype> incident_commands(source_t key);


      // graphics
      void draw_universe();
    };
  };
};
#endif
