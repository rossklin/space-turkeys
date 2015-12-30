#ifndef _STK_COST
#define _STK_COST

#include "types.h"

namespace st3{
  namespace cost{

    /* type allocation templates */
    template<typename T>
    struct resource_allocation{
      T organics;
      T metals;
      T gases;
    };

    template<typename T>
    struct sector_allocation{
      T agriculture;
      T infrastructure;
      T research;
      T culture;
      T military;
    };

    template<typename T>
    struct ship_allocation{
      T scout;
      T fighter;
      T bomber;
      T colonizer;
    };
    
    struct resource_data{
      float initial;
      float available;
      float storage;
      float occupied;

      resource_data();
    };

    typedef resource_allocation<sfloat> resource_base;
    typedef sector_allocation<sfloat> sector_base;
    typedef ship_allocation<sfloat> ship_base;

    struct sector_cost{
      resource_base res;
      sfloat water;
      sfloat space;
      sfloat time;
    };

    struct ship_cost{
      resource_base res;
      sfloat time;
    };

    /*! cost for sector expansion */
    extern sector_allocation<sector_cost> sector_expansion;

    /*! cost for housing expansion */
    extern sector_cost housing;

    /*! cost for ship */
    extern ship_allocation<ship_cost> ship_build;

    void initialize();
  };
};
#endif
