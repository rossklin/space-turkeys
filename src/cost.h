#ifndef _STK_COST
#define _STK_COST

#include "types.h"

namespace st3{
  namespace cost{

    struct sector_cost{
      vmap capacity;
      vmap usage;
    };

    extern hm_t<std::string, sector_cost> sector; 
    extern hm_t<std::string, vmap> ship;
    extern hm_t<std::string, vmap> defense;
    extern hm_t<std::string, vmap> research;

    void initialize();
  };
};
#endif
