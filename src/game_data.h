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
  namespace server {
    struct client_t;
  };

  struct interaction_info {
    combid source;
    combid target;
    std::string interaction;
  };

  class entity_package {
  public: 
    hm_t<idtype, player> players; /*!< table of players */
    game_settings settings; /*! game settings */
    hm_t<combid, game_object::ptr> entity;
    std::list<combid> remove_entities; 
    hm_t<idtype, std::set<combid> > evm;

    void clear_entities();
    game_object::ptr get_entity(combid i);
    void limit_to(idtype pid);
    void copy_from(const game_data &g);
    std::list<game_object::ptr> all_owned_by(idtype pid);
  };

  /*! struct containing data for game objects */
  class game_data : public entity_package{
  public:
    static void confirm_data();
    
    grid::tree::ptr entity_grid;
    std::vector<interaction_info> interaction_buffer;
    std::set<id_pair> collision_buffer;

    game_data();
    ~game_data();
    game_data(const game_data &g) = delete;
    void assign(const game_data &g);
    void apply_choice(choice::choice c, idtype id);
    void increment();
    void collide_ships(id_pair x);
    bool target_position(combid t, point &p);
    std::list<combid> search_targets(combid self_id, point p, float r, target_condition c);
    std::list<combid> search_targets_nophys(combid self_id, point p, float r, target_condition c);
    void rebuild_evm();

    // access
    ship::ptr get_ship(combid i);
    fleet::ptr get_fleet(combid i);
    solar::ptr get_solar(combid i);
    waypoint::ptr get_waypoint(combid i);

    template<typename T>
    std::list<typename T::ptr> all();

    void add_entity(game_object::ptr p);
    void remove_units();
    void generate_fleet(point p, idtype i, command &c, std::list<combid> &sh);
    void relocate_ships(command c, std::set<combid> &sh, idtype owner);

    // game steps
    void pre_step(); 
    void end_step();
    void build_players(hm_t<int, server::client_t*> clients);
    void build();

  protected:
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
