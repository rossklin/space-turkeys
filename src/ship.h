#ifndef _STK_SHIP
#define _STK_SHIP

#include <string>
#include <vector>

#include "types.h"

namespace st3{
  struct research;

  /*! ship game object */
  struct ship{
    /*! counter for ship ids */
    static idtype id_counter;

    /*! type for ship class identifier */
    typedef sint class_t;

    // serialised variables

    idtype fleet_id; /*!< id of the ship's fleet */
    class_t ship_class; /*!< ship class */
    point position; /*!< ship's position */
    sfloat angle; /*!< ship's angle */
    sfloat speed; /*!< ship's speed */
    sint owner; /*!< id of the ship's owner */
    sint hp; /*!< ship's hit points */
    sfloat interaction_radius; /*!< radius in which the ship can fire */

    // buffer and mechanical data

    sfloat vision; /*!< ship's sight radius */
    bool was_killed; /*!< track when the ship is killed */

    /*! default constructor */
    ship();

    /*! build a ship of given class and research level
      @param c ship class
      @param r research level
    */
    ship(class_t c, research &r);
  };
};
#endif