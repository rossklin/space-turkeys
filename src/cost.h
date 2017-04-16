#ifndef _STK_COST
#define _STK_COST

#include "types.h"

namespace st3{
  namespace cost{
    namespace keywords{
      extern const std::vector<std::string> resource;
      extern const std::vector<std::string> sector;
      extern const std::vector<std::string> expansion;
      extern const std::vector<std::string> ship;
      extern const std::vector<std::string> turret;

      const std::string key_metals = "metals";
      const std::string key_organics = "organics";
      const std::string key_gases = "gases";
      const std::string key_research = "research";
      const std::string key_culture = "culture";
      const std::string key_mining = "mining";
      const std::string key_military = "military";
      const std::string key_expansion = "expansion";
      const std::string key_scout = "scout";
      const std::string key_fighter = "fighter";
      const std::string key_bomber = "bomber";
      const std::string key_colonizer = "colonizer";
      const std::string key_radar_turret = "radar turret";
      const std::string key_rocket_turret = "rocket turret";
    };
    
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
      countable_resource_allocation<sfloat> res;
      sfloat water;
      sfloat space;
      sfloat time;
    };

    struct ship_cost{
      countable_resource_allocation<sfloat> res;
      sfloat time;
    };

    struct turret_cost{
      countable_resource_allocation<sfloat> res;
      sfloat time;
    };

    /*! cost for sector expansion */
    sector_allocation<sector_cost> &sector_expansion();

    /*! cost for ship */
    ship_allocation<ship_cost> &ship_build();

    /*! cost for turret */
    turret_allocation<turret_cost> &turret_build();

    float expansion_multiplier(float level);
  };
};
#endif
