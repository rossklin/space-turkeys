#ifndef _STK_COST
#define _STK_COST

#include "types.h"

namespace st3{
  namespace cost{
    namespace keywords{
      extern const std::vector<std::string> resource;
      extern const std::vector<std::string> sector;
      extern const std::vector<std::string> ship;
      extern const std::vector<std::string> turret;
    };
    
    /* Templated string hash map which will complain when attempting
       to access a non-initialized key. Use together with keywords. */
    template<typename T>
    struct allocation{
      hm_t<std::string, T> data;

      void setup(std::vector<std::string> x);
      bool confirm_content(std::vector<std::string> x);
      T& operator[] (const std::string& k);
    };

    template<typename T>
    struct countable_allocation : public allocation<T>{
      T count();
    };
        
    struct resource_data{
      float available;
      float storage;
    };

    struct sector_cost{
      allocation<sfloat> res;
      sfloat water;
      sfloat space;
      sfloat time;

      sector_cost();
    };

    struct ship_cost{
      allocation<sfloat> res;
      sfloat time;

      ship_cost();
    };

    struct turret_cost{
      allocation<sfloat> res;
      sfloat time;

      turret_cost();
    };

    /*! cost for sector expansion */
    extern allocation<sector_cost> sector_expansion;

    /*! cost for ship */
    extern allocation<ship_cost> ship_build;

    /*! cost for turret */
    extern allocation<turret_cost> turret_build;

    void initialize();
  };
};
#endif
