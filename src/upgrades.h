#ifndef _STK_UPGRADES
#define _STK_UPGRADES

#include <string>
#include <list>
#include <set>
#include <functional>

#include "interaction.h"

namespace st3{
  class upgrade{
  public:
    typedef std::function<void(ship::ptr)> modify_t;
    
    static constexpr std::string space_combat = "space combat";
    static constexpr std::string bombard = "bombard";
    static constexpr std::string colonize = "colonize";
    
    static upgrade table(string k);

    std::list<interaction> inter;
    modify_t modify;
    std::set<std::string> exclusive;
  };
};

#endif
