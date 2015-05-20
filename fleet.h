#ifndef _STK_FLEET
#define _STK_FLEET

#include <set>
#include "types.h"
#include "command.h"

namespace st3{
  /*! a fleet controls a set of ships */
  struct fleet{
    static idtype id_counter; /*!< id counter for fleets */
    static const int update_period = 1; /*!< number of increments between fleet data updates */
    static const int interact_d2 = 100; /*!< distance from target at which the fleet converges */
    static constexpr float min_radius = 10; /*!< smallest allowed fleet radius (for visibility) */

    // serialized components
    std::set<idtype> ships; /*!< ids of ships in fleet */
    command com; /*!< the fleet's command (currently this only holds the target) */
    point position; /*!< current estimated position */
    sfloat radius; /*!< current estimated radius */
    sfloat vision; /*!< current vision range */
    sint owner; /*!< id of player owning the fleet */
    sbool converge; /*!< whether the fleet is converging on it's target */

    // mechanical components
    int update_counter; /*!< counter for updating fleet data */
    float speed_limit; /*!< speed of slowest ship in fleet */
    
    /*! default constructor */
    fleet();

    /*! whether the fleet is idle i.e. at a waypoint or missing it's target */
    bool is_idle();
  };
};
#endif
