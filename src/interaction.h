#ifndef _STK_INTERACTION
#define _STK_INTERACTION

#include "game_object.h"

namespace st3{
  class target_condition{
  public:
    const sint neutral = 0;
    const sint owned = 1;
    const sint enemy = 2;
      
    class_t what;
    sint alignment;
    idtype owner;

    target_condition(idtype o, sint a, class_t w);
  };

  class interaction{
  public:
    static bool valid(target_condition c, game_object::ptr t);
  };
};

#endif
