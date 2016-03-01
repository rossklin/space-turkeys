#ifndef _STK_SHIP
#define _STK_SHIP

#include <string>
#include <set>

#include "types.h"
#include "game_object.h"

namespace st3{
  class game_data;

  /*! ship game object */
  class ship : public virtual game_object{
  public:
    typedef std::shared_ptr<ship> ptr;
    static ptr create();
    
    struct stats{
      sfloat speed; /*!< ship's speed */
      sint hp; /*!< ship's hit points */
      sfloat accuracy;
      sfloat ship_damage;
      sfloat solar_damage;
      sfloat interaction_radius; /*!< radius in which the ship can fire */
      sfloat vision; /*!< ship's sight radius */
      sfloat load_time;
    };
    
    struct target_condition{
      identifier::class_t what;
      sint status;
      const sint owned = 1;
      const sint neutral = 2;
      const sint enemy = 4;
    };

    identifier::class_t ship_class; /*!< ship class */
    sfloat angle; /*!< ship's angle */
    sfloat load;
    stats base_stats;
    stats current_stats;
    std::set<std::string> upgrades;    

    ship();
    ~ship();
    void pre_phase(game_data *g);
    void move(game_data *g);
    void interact(game_data *g);
    void post_phase(game_data *g);
    stats compile_stats();
    float vision();

  protected:
    // serialised variables
    identifier::combid fleet_id; /*!< id of the ship's fleet */
    std::function<void(game_object::ptr from, ship::ptr self, float damage)> receive_damage;
  };
};
#endif
