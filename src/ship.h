#ifndef _STK_SHIP
#define _STK_SHIP

#include <string>
#include <set>

#include "types.h"
#include "game_object.h"
#include "interaction.h"
#include "cost.h"
#include "ship_stats.h"

namespace st3{
  class game_data;
  class solar;

  /*! ship game object */
  class ship : public virtual physical_object, public ship_stats {
  public:
    typedef ship* ptr;
    static ptr create();
    static const std::string class_id;
    static std::list<std::string> all_classes();
    static std::string starting_ship;
    static const int na;

    combid fleet_id; /*!< id of the ship's fleet */
    sfloat angle; /*!< ship's angle */
    sfloat load;
    ship_stats base_stats;
    sint passengers;
    sint is_landed;
    cost::res_t cargo;

    // ai stats
    float target_angle;
    float target_speed;
    bool activate;
    bool force_refresh;
    std::list<combid> neighbours;
    std::list<combid> local_enemies;
    std::list<combid> local_friends;
    std::list<combid> local_all;
    std::vector<float> free_angle;
    bool check_space(float a);

    // game_object
    void pre_phase(game_data *g);
    void move(game_data *g);
    void post_phase(game_data *g);
    void on_remove(game_data *g);
    float vision();
    bool serialize(sf::Packet &p);
    bool is_active();
    ptr clone();
    bool isa(std::string c);
  
    // physical_object
    std::set<std::string> compile_interactions();
    float interaction_radius();
    bool can_see(game_object::ptr x);

    // ship
    ship();
    ship(const ship &s);
    ship(const ship_stats &s);
    ~ship();
    
    void update_data(game_data *g);
    void set_stats(ship_stats s);
    void receive_damage(game_object::ptr from, float damage);
    void on_liftoff(solar *from, game_data *g);
    bool has_fleet();
    float evasion_check();
    float accuracy_check(ship::ptr a);

  protected:
    // serialised variables
    virtual game_object::ptr clone_impl();
    void copy_from(const ship &s);
  };
};
#endif
