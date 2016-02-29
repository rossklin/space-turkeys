#ifndef _STK_GAMEDATA
#define _STK_GAMEDATA

#include <SFML/Network.hpp>

#include "types.h"
#include "game_object.h"
#include "ship.h"
#include "fleet.h"
#include "waypoint.h"
#include "solar.h"
#include "choice.h"
#include "player.h"
#include "grid_tree.h"
#include "game_settings.h"
#include "interaction.h"

namespace st3{

  /*! struct containing data for game objects */
  class game_data{
  public:
    hm_t<combid, game_object::ptr> entity;
    hm_t<idtype, player> players; /*!< table of players */
    game_settings settings; /*! game settings */

    game_data();
    void apply_choice(choice::choice c, idtype id);
    game_data limit_to(idtype pid);
    void increment();
    void remove_units();
    bool entity_seen_by(combid id, idtype pid);

    // access
    ship::ptr get_ship(combid i);
    fleet::ptr get_fleet(combid i);
    solar::ptr get_solar(combid i);
    waypoint::ptr get_waypoint(combid i);

    combid entity_at(point p);
    std::list<combid> search_targets(point p, float r, interaction::target_condition c);

  protected:
    std::list<combid> remove_entities; 
    grid::tree::ptr object_grid;

    bool target_position(combid t, point &p);
    void generate_fleet(point p, idtype i, command &c, std::set<combid> &sh);
    void relocate_ships(command &c, std::set<combid> &sh, idtype owner);
    void set_solar_commands(combid id, std::list<command> coms);
    void set_fleet_commands(combid id, std::list<command> coms);

    // object iteration phases
    void pre_phase();
    void update();
    void post_phase();

    // add and remove entities
    void add_entity(game_object::ptr p);
    void remove_entity(combid id);

    // game steps
    void pre_step(); 
    void end_step(); 
    void build();

    // building universe
    hm_t<idtype, solar::solar> random_solars();
    float heuristic_homes(hm_t<idtype, solar::solar> solar_buf, hm_t<idtype, idtype> &start_solars);
  };
};
#endif
