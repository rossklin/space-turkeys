#ifndef _STK_UPGRADES
#define _STK_UPGRADES

#include <string>
#include <set>
#include <functional>

#include "interaction.h"
#include "ship.h"
#include "solar.h"

namespace st3{
  class game_data;
  
  class upgrade{
  public:
    
    static const hm_t<std::string, upgrade> &table();

    std::set<std::string> inter;
    std::set<std::string> on_liftoff;
    ssmod_t modify;
  };
};

#endif
