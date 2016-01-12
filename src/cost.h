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
      void confirm_content(std::vector<std::string> x);
      T& operator[] (const std::string& k);
    };

    template<typename T>
    struct countable_allocation : public virtual allocation<T>{
      T count();
    };

    /*! base allocation classes */
    template<typename T>
    struct ship_allocation : public virtual allocation<T>{
      ship_allocation();
    };

    template<typename T>
    struct turret_allocation : public virtual allocation<T>{
      turret_allocation();
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
    struct countable_ship_allocation : public virtual ship_allocation<T>, public virtual countable_allocation<T>{
    };

    template<typename T>
    struct countable_turret_allocation : public virtual turret_allocation<T>, public virtual countable_allocation<T>{
    };

    template<typename T>
    struct countable_sector_allocation : public virtual sector_allocation<T>, public virtual countable_allocation<T>{
    };

    template<typename T>
    struct countable_resource_allocation : public virtual resource_allocation<T>, public virtual countable_allocation<T>{
    };

    // cost structures
    struct resource_data{
      float available;
      float storage;
    };

    struct sector_cost{
      resource_allocation<sfloat> res;
      sfloat water;
      sfloat space;
      sfloat time;
    };

    struct ship_cost{
      resource_allocation<sfloat> res;
      sfloat time;
    };

    struct turret_cost{
      resource_allocation<sfloat> res;
      sfloat time;
    };

    /*! cost for sector expansion */
    extern sector_allocation<sector_cost> sector_expansion;

    /*! cost for ship */
    extern ship_allocation<ship_cost> ship_build;

    /*! cost for turret */
    extern turret_allocation<turret_cost> turret_build;    

    void initialize();
  };
};
#endif
