#ifndef _STK_COST
#define _STK_COST

#include "types.h"

namespace st3{
  namespace cost{
    
    struct allocation{
      hm_t<std::string, sfloat> data;

      void setup(std::vector<std::string> x);
      void confirm_content(std::vector<std::string> x);
      sfloat count();
      void normalize();
      void add(allocation a);
      void scale(float a);

      sfloat& operator[] (const std::string& k);
    };

    struct ship_allocation : public allocation {
      ship_allocation();
    };

    struct sector_allocation : public allocation {
      sector_allocation();
    };

    struct resource_allocation : public allocation{
      resource_allocation();
    };

    typedef resource_allocation res_t;

    struct dev_cost{
      res_t res;
      sfloat time;

      dev_cost();
    };

    float expansion_multiplier(float level);
  };
};
#endif
