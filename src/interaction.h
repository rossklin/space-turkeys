#ifndef _STK_INTERACTION
#define _STK_INTERACTION

#include "game_object.h"

namespace st3{
  class game_data;
  
  class target_condition{
  public:
    static const sint neutral = 1;
    static const sint owned = 1 << 1;
    static constexpr sint enemy = 1 << 2;
    static constexpr sint any_alignment = -1;
    static const class_t no_target;

    class_t what;
    sint alignment;
    idtype owner;

    target_condition();
    target_condition(sint a, class_t w);
    target_condition owned_by(idtype o);
    bool requires_target();
    bool valid_on(game_object::ptr o);

    static bool get_alignment(idtype t, idtype s);
  };

  class interaction{
  public:
    typedef std::function<void(game_object::ptr self, game_object::ptr target, game_data *g)> perform_t;
    static hm_t<std::string, interaction> &table();

    std::string name;
    target_condition condition;
    perform_t perform;
  };
};

#endif
