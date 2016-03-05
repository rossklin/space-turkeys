#ifndef _STK_INTERACTION
#define _STK_INTERACTION

#include "game_object.h"

namespace st3{
  class target_condition{
  public:
    static const sint neutral = 0;
    static const sint owned = 1;
    static const sint enemy = 2;
      
    class_t what;
    sint alignment;
    idtype owner;

    target_condition();
    target_condition(sint a, class_t w);
    target_condition(idtype o, sint a, class_t w);
    target_condition owned_by(idtype o);
  };

  class interaction{
  public:
    typedef std::function<void(game_object::ptr self, game_object::ptr target)> perform_t;
    static bool valid(target_condition c, game_object::ptr t);

    std::string name;
    target_condition condition;
    perform_t perform;
  };
};

#endif
