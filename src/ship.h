#ifndef _STK_SHIP
#define _STK_SHIP

#include <string>
#include <vector>

#include "types.h"
#include "cost.h"

namespace st3{
  /*! ship game object */
  struct ship{
    /*! counter for ship ids */
    static idtype id_counter;

    /*! type for ship class identifier */
    typedef std::string class_t;

    struct target_condition{
      source_t what;
      sint status;
      const sint owned = 1;
      const sint neutral = 2;
      const sint enemy = 4;
    };

    // interaction types
    typedef std::function<void(ship*,ship*,game_data*)> shipi_t;
    typedef std::function<void(ship*,solar*,game_data*)> solari_t;

    // serialised variables
    idtype fleet_id; /*!< id of the ship's fleet */
    class_t ship_class; /*!< ship class */
    point position; /*!< ship's position */
    sfloat angle; /*!< ship's angle */
    sfloat speed; /*!< ship's speed */
    sint owner; /*!< id of the ship's owner */
    sint hp; /*!< ship's hit points */
    sfloat interaction_radius; /*!< radius in which the ship can fire */
    sfloat vision; /*!< ship's sight radius */
    sfloat load_time;
    sfloat load;
    hm_t<target_condition, std::string> interaction_list;

    // buffer and mechanical data
    bool remove; /*!< track when the ship is killed */

    // interactions
    hm_t<std::string, shipi_t> ship_interaction;
    hm_t<std::string, solari_t> > solar_interaction;
    std::function<void(ship*, ship*, float)> receive_damage;
  };
};
#endif
