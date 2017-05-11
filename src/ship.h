#ifndef _STK_SHIP
#define _STK_SHIP

#include <string>
#include <set>

#include "types.h"
#include "game_object.h"
#include "interaction.h"
#include "cost.h"

namespace st3{
  class game_data;
  class solar;
    
  struct ship_stats{
    sfloat speed; /*!< ship's speed */
    sfloat hp; /*!< ship's hit points */
    sfloat accuracy;
    sfloat ship_damage;
    sfloat solar_damage;
    sfloat interaction_radius; /*!< radius in which the ship can fire */
    sfloat vision; /*!< ship's sight radius */
    sfloat load_time;

    ship_stats operator += (const ship_stats &s);
    ship_stats();
  };

  /*! ship game object */
  class ship : public virtual physical_object{
  public:
    typedef ship* ptr;
    static ptr create();
    static const std::string class_id;

    // ship class info
    class_t ship_class; /*!< ship class */
    std::string depends_tech;
    sint depends_facility_level;

    combid fleet_id; /*!< id of the ship's fleet */
    sfloat angle; /*!< ship's angle */
    sfloat load;
    ship_stats base_stats;
    ship_stats current_stats;
    std::set<std::string> upgrades;
    cost::resource_allocation<float> cargo;
    sfloat cargo_capacity;
    sint is_landed;

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
    std::list<combid> confirm_interaction(std::string a, std::list<combid> t, game_data *g);
    std::set<std::string> compile_interactions();
    float interaction_radius();

    // ship
    ship();
    ~ship();

    void receive_damage(game_object::ptr from, float damage);
    void on_liftoff(solar *from, game_data *g);
    bool has_fleet();
    bool accuracy_check(float a, ship::ptr t);

  protected:
    // serialised variables
    virtual game_object::ptr clone_impl();
    void copy_from(const ship &s);
  };
};
#endif
