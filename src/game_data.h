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
    std::list<combid> remove_entities; 
    grid::tree::ptr entity_grid;

    game_data();
    game_data(const game_data &g);
    game_data &operator =(const game_data &g);
    void apply_choice(choice::choice c, idtype id);
    game_data limit_to(idtype pid);
    void increment();
    bool entity_seen_by(combid id, idtype pid);
    bool target_position(combid t, point &p);
    combid entity_at(point p);
    std::list<combid> search_targets(point p, float r, target_condition c);

    // access
    ship::ptr get_ship(combid i);
    fleet::ptr get_fleet(combid i);
    solar::ptr get_solar(combid i);
    waypoint::ptr get_waypoint(combid i);
    game_object::ptr get_entity(combid i);

    template<typename T>
    std::list<typename T::ptr> all();
    std::list<game_object::ptr> all_owned_by(idtype pid);

    void add_entity(game_object::ptr p);
    void remove_units();
    void generate_fleet(point p, idtype i, command &c, std::list<combid> &sh);
    void relocate_ships(command &c, std::set<combid> &sh, idtype owner);

    // game steps
    void pre_step(); 
    void end_step(); 
    void build();

  protected:    
    bool validate_choice(choice::choice c, idtype id);

    // object iteration phases
    void pre_phase();
    void update();
    void post_phase();

    // add and remove entities
    void remove_entity(combid id);
    void distribute_ships(std::list<combid> sh, point p);
    void allocate_grid();
  };
};
#endif
