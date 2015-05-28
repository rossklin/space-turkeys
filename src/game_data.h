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

  /*! struct containing data for game objects */
  struct game_data{
    hm_t<idtype, ship> ships; /*< table of ships */
    hm_t<idtype, fleet> fleets; /*!< table of fleets */
    hm_t<source_t, waypoint> waypoints; /*!< table of waypoints */
    hm_t<idtype, solar::solar> solars; /*!< table of solars */
    hm_t<idtype, player> players; /*!< table of players */
    hm_t<idtype, solar::choice_t> solar_choices; /*!< table of choices for solars */
    std::list<source_t> remove_entities; /*! list of entities to remove to send to clients */
    game_settings settings; /*! game settings */
    grid::tree *ship_grid; /*! grid search tree to track ship positions */
    
    /* **************************************** */
    /* APPLY CHOICE */
    /* **************************************** */

    /*! apply a choice

      Add/update waypoints and solar choices. Set fleet and solar
      commands, possibly generating new fleets.

      @param c the choice
      @param id the client id
    */
    void apply_choice(choice c, idtype id);

    /*! try to get the position of a target

      This will fail if the target is idle.

      @param t target key
      @param[out] p the position
      @return whether a target position was found
    */
    bool target_position(target_t t, point &p);

    /*! generate a fleet at a position
      
      Generates a fleet with given position, owner, command and ship
      ids. The ships are repositioned with gaussian distribution about
      the fleet position, and inserted in the ship grid.

      @param p position
      @param i id of the player owning the fleet
      @param c command for the fleet
      @param sh set of ship ids
    */
    void generate_fleet(point p, idtype i, command &c, std::set<idtype> &sh);

    /*! relocate ships to a new fleet
      
      Generates a new fleet and moves some ships into it. Any fleets
      emptied by this process are removed. In the special case where
      the ships to be relocated fill exactly one fleet, it's fleet id
      is reused so that any fleet targeting it can continue doing so.

      @param c command for the new fleet
      @param sh set of ids of ships to relocate
      @param owner id of the player who should own the new fleet
    */
    void relocate_ships(command &c, std::set<idtype> &sh, idtype owner);
    
    /*! Generate fleets from a solar for given commands

      Generates a new fleet for each command in the list.

      @param id id of the solar
      @param coms list of commands for new fleets
    */
    void set_solar_commands(idtype id, std::list<command> coms);

    /*! Split a fleet with given commands

      Generates a new fleet for each command in the list, unless a
      command allocates all ships in the fleet, in which case no new
      fleet is generated and the command is given to the original
      fleet instead.

      @param id id of the fleet to split
      @param coms list of commands for the new fleets
    */
    void set_fleet_commands(idtype id, std::list<command> coms);

    /* **************************************** */
    /* ITERATION */
    /* **************************************** */

    /*! initialize ship grid */
    void allocate_grid(); 

    /*! deallocate ship grid */
    void deallocate_grid(); 

    /*! take one step in game mechanices */
    void increment(); 

    /*! get the id of the solar at a point
      @param p point to check
      @return id of the solar, or -1 if no solar was found
    */
    idtype solar_at(point p);
    
    /*! land a ship at a solar

      Add the ship to the solar, remove it from the fleet and the ship
      grid. If the ship's fleet is left empty, remove the fleet. 

      @param ship_id id of ship to land
      @param solar_id id of solar to land at
    */
    void ship_land(idtype ship_id, idtype solar_id);
    
    /*! colonize a solar

      @param ship_id id of colonizer ship to use
      @param solar_id id of solar to colonize
    */
    void ship_colonize(idtype ship_id, idtype solar_id);

    /*! let a ship bombard a solar

      Damage the defense and population of the solar. If the defense
      is reduced to zero, change the owner of the solar to be the
      owner of the ship and (tempfix) add some population to the
      solar. Then remove the ship.

      @param ship_id id of the bombarding ship
      @param solar_id id of the bombarded solar
    */
    void ship_bombard(idtype ship_id, idtype solar_id);

    /*! let a ship fire on a ship
      
      Let a ship fire on another ship, reducing it's hp. Set
      was_killed on the targeted ship if it's hp go below 0. 

      @param s id of the firing ship
      @param t id of the targeted ship
      @return whether ship s gets rapidfire and can fire again
    */
    bool ship_fire(idtype s, idtype t);

    /*! remove a ship
      
      Removes a ship from it's fleet, from the ship grid and from the
      ship table. Removes the fleet if empty.

      @param id id of the ship to remove
    */
    void remove_ship(idtype id);

    /*! remove a fleet
      
      Removes a fleet from the fleet table and adds a corresponding
      key to remove_entities.

      @param id id of the fleet to remove
    */
    void remove_fleet(idtype id);

    /*! update a solar system
      
      Updates the population, industry etc. of a solar by one time
      step settings.dt.

      @param id id of the solar system to update
    */
    void solar_tick(idtype id);

    /*! update a fleet

      Updates the position, radius, vision and converge/idle status of
      a fleet.

      @param id id of the fleet to update
    */
    void update_fleet_data(idtype id);

    /* **************************************** */
    /* GAME ROUND STEPS */
    /* **************************************** */

    /*! before loading choices step
      
      Clean up things that will be reloaded from the client's choices:
      set the targets of fleets to idle and remove all waypoints.
    */
    void pre_step(); 

    /*! after simulation step

      Remove all waypoints that have no incoming commands.
    */
    void end_step(); 

    /* **************************************** */
    /* BUILD STUFF */
    /* **************************************** */

    /*! build a universe

      Initialize the game data with random solars. Players must already be set.
    */
    void build();

    /*! generate random solars according to game settings
      @return table of solars
    */
    hm_t<idtype, solar::solar> random_solars();

    /*! find fairest choice of starting solars for players
      @param solar_buf table of solars to use
      @param[out] start_solars fairest identified selection of starting solars
      @return unfairness estimate in range [0, num_players]
    */
    float heuristic_homes(hm_t<idtype, solar::solar> solar_buf, hm_t<idtype, idtype> &start_solars);

    /*! check whether a fleet is in sight for a player
      @param fid id of the fleet
      @param pid id of the player
      @return whether the fleet is seen
    */
    bool fleet_seen_by(idtype fid, idtype pid);

    /*! compute a sight-limited copy of the game data

      Computes a game data object with only those entities visible to
      a given player. Used when sending game data to the clients.

      @param pid id of the player
      @return vision limited game data
    */
    game_data limit_to(idtype pid);
    
  };
};
#endif
