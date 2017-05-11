#ifndef _STK_COST
#define _STK_COST

#include "types.h"

namespace st3{
  namespace cost{
    
    /* Templated string hash map which will complain when attempting
       to access a non-initialized key. Use together with keywords. 
       
       Note that you can't trivially use std::move on these
       allocations, since (gcc docs):

       -Wno-virtual-move-assign
           Suppress warnings about inheriting from a virtual base with a non-trivial C++11 move assignment operator.  This is dangerous because if
           the virtual base is reachable along more than one path, it is moved multiple times, which can mean both objects end up in the moved-
           from state.  If the move assignment operator is written to avoid moving from a moved-from object, this warning can be disabled.
    */
    template<typename T>
    struct allocation{
      hm_t<std::string, T> data;

      virtual void setup(std::vector<std::string> x);
      void confirm_content(std::vector<std::string> x);
      T& operator[] (const std::string& k);
    };

    template<typename T>
    struct countable_allocation : public virtual allocation<T>{
      void setup(std::vector<std::string> x) override;
      T count();
      void normalize();
      void add(countable_allocation<T> a);
      void scale(float a);
    };

    /*! base allocation classes */
    template<typename T>
    struct ship_allocation : public virtual allocation<T>{
      ship_allocation();
    };

    template<typename T>
    struct sector_allocation : public virtual allocation<T>{
      sector_allocation();
    };

    template<typename T>
    struct resource_allocation : public virtual allocation<T>{
      resource_allocation();
    };

    /*! countable base allocation classes */
    template<typename T>
    struct countable_ship_allocation : public virtual ship_allocation<T>, public virtual countable_allocation<T>{};

    template<typename T>
    struct countable_sector_allocation : public virtual sector_allocation<T>, public virtual countable_allocation<T>{};

    template<typename T>
    struct countable_resource_allocation : public virtual resource_allocation<T>, public virtual countable_allocation<T>{};

    typedef countable_resource_allocation<sfloat> res_t;

    struct facility_cost{
      res_t res;
      sfloat water;
      sfloat space;
      sfloat time;
    };

    struct ship_cost{
      res_t res;
      sfloat time;
    };

    /*! cost for ship */
    ship_allocation<ship_cost> &ship_build();
    float cost_multiplier(float level);
  };
};
#endif
