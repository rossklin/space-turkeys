#ifndef _STK_UPGRADES
#define _STK_UPGRADES

#include <string>
#include <set>

#include "interaction.h"
#include "ship.h"

namespace st3{
  class upgrade{
  public:    
    static hm_t<std::string, upgrade> &table();

    std::set<std::string> inter;
    ship_stats modify;
    std::set<std::string> exclusive;
  };
};

#endif
