#ifndef _STK_INTERACTION
#define _STK_INTERACTION

#include "game_object.h"

namespace st3{
  class target_condition{
  public:
    static const sint neutral = 1;
    static const sint owned = 1 << 1;
    static constexpr sint enemy = 1 << 2;
    static constexpr sint any_alignment = -1;
    static const class_t no_target;

    class_t macro_target;
    class_t what;
    sint alignment;
    idtype owner;

    target_condition();
    target_condition(sint a, class_t w, class_t m = "");
    target_condition owned_by(idtype o);
    bool requires_target();

    static bool get_alignment(idtype t, idtype s);
  };

  class interaction{
  public:
    typedef std::function<void(game_object *self, game_object *target)> perform_t;
    static bool valid(target_condition c, game_object::ptr t);
    static bool macro_valid(target_condition c, game_object::ptr t);
    static hm_t<std::string, interaction> &table();

    std::string name;
    target_condition condition;
    perform_t perform;
  };
};

#endif
