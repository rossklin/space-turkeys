#ifndef _STK_GAMEDATA
#define _STK_GAMEDATA

#include <SFML/Network.hpp>

#include "types.h"
#include "ship.h"
#include "fleet.h"
#include "waypoint.h"
#include "solar.h"
#include "choice.h"
#include "player.h"
#include "grid_tree.h"
#include "game_settings.h"

namespace st3{
  struct game_data{
    hm_t<idtype, ship> ships;
    hm_t<idtype, fleet> fleets;
    hm_t<source_t, waypoint> waypoints;
    hm_t<idtype, solar::solar> solars;
    hm_t<idtype, player> players;
    hm_t<idtype, solar::choice_t> solar_choices;
    game_settings settings;
    grid::tree *ship_grid;
    float dt;
    
    // apply_choice
    void apply_choice(choice c, idtype id);
    bool target_position(target_t t, point &p);
    void generate_fleet(point p, idtype i, command &c, std::set<idtype> &sh);
    void relocate_ships(command &c, std::set<idtype> &sh, idtype owner);
    void set_solar_commands(idtype id, std::list<command> coms);
    void set_fleet_commands(idtype id, std::list<command> coms);

    // iteration
    void allocate_grid(); // initialise ship_grid
    void deallocate_grid(); // delete ship_grid
    void increment(); // take one step in game mechanics
    idtype solar_at(point p); // id of solar at point p, or -1 if no solar
    void ship_land(idtype ship_id, idtype solar_id);
    void ship_bombard(idtype ship_id, idtype solar_id);
    bool ship_fire(idtype s, idtype t);
    void remove_ship(idtype id);
    void solar_tick(idtype id);
    void update_fleet_data(idtype id);

    // game round steps

    // before choice: idle fleets and clear waypoint keep_me
    void pre_step(); 
    // after evolution: remove unused waypoints
    void end_step(); 

    // build stuff
    void build();
    hm_t<idtype, solar::solar> random_solars();
    float heuristic_homes(hm_t<idtype, solar::solar> solar_buf, hm_t<idtype, idtype> &start_solars);
    game_data limit_to(idtype pid);
    
  };
};
#endif
