#ifndef _STK_GAMEDATA
#define _STK_GAMEDATA

#include <SFML/Network.hpp>

#include "types.h"
#include "ship.h"
#include "fleet.h"
#include "solar.h"
#include "choice.h"
#include "player.h"
#include "grid_tree.h"
#include "game_settings.h"

namespace st3{
  struct game_data{
    hm_t<idtype, ship> ships;
    hm_t<idtype, fleet> fleets;
    hm_t<idtype, solar> solars;
    hm_t<idtype, player> players;
    game_settings settings;
    grid::tree *ship_grid;

    // apply_choice
    void apply_choice(choice c, idtype id);
    point target_position(target_t t);
    void generate_fleet(point p, idtype i, command c);
    void set_solar_commands(idtype id, std::list<command> coms);
    void set_fleet_commands(idtype id, std::list<command> coms);

    // iteration
    void allocate_grid();
    void deallocate_grid();
    void increment();
    void cleanup(); // remove dead ships
    idtype solar_at(point p);
    void ship_land(idtype ship_id, idtype solar_id);
    void ship_bombard(idtype ship_id, idtype solar_id);
    bool ship_fire(idtype s, idtype t);
    void remove_ship(idtype id);

    // build stuff
    void build();
    hm_t<idtype, solar> random_solars();
    float heuristic_homes(hm_t<idtype, solar> solar_buf, hm_t<idtype, idtype> &start_solars);
  };
};
#endif
