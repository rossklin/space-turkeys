#ifndef _STK_TURRET
#define _STK_TURRET

#include <string>
#include <vector>

#include "cost.h"
#include "types.h"

namespace st3 {
/*! turret game object */
struct turret {
  sfloat range;  /*!< radius in which the turret can fire */
  sfloat damage; /*!< turret's damage */
  sfloat accuracy;
  sfloat load_time;
};
};  // namespace st3
#endif
