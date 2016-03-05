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
    
    static const std::string space_combat;
    static const std::string bombard;
    static const std::string colonize;
    
    static upgrade table(std::string k);

    std::list<interaction> inter;
    modify_t modify;
    std::set<std::string> exclusive;
  };
};

#endif
