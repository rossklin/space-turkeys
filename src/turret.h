#ifndef _STK_TURRET
#define _STK_TURRET

#include <string>
#include <vector>

#include "types.h"
#include "cost.h"

namespace st3{
  /*! turret game object */
  struct turret{

    /*! type for turret class identifier */
    typedef std::string class_t;

    // serialised variables

    class_t turret_class; /*!< turret class */
    sfloat range; /*!< radius in which the turret can fire */
    sfloat vision; /*!< turret's sight radius */
    sfloat damage; /*!< turret's damage */
    sfloat accuracy;
    sfloat load_time;
    sfloat load;
    sfloat hp;
  };
};
#endif
