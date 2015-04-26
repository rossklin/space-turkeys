#ifndef _STK_SOLAR
#define _STK_SOLAR

#include <set>
#include <vector>
#include "types.h"
#include "ship.h"

namespace st3{
  struct research;

  namespace solar{
    extern idtype id_counter;

    // enums for development data
    enum d_population{
      p_research = 0,
      p_industry,
      p_resource,
      p_num
    };

    enum d_industry{
      i_infrastructure = 0,
      i_agriculture,
      i_ship,
      i_research,
      i_resource,
      i_num
    };

    enum d_resource{
      o_metal = 0,
      o_gas,
      o_num
    };

    enum d_ship{
      s_scout = 0,
      s_fighter,
      s_num
    };

    extern const float research_per_person;
    extern const float industry_per_person;
    extern const float fleet_per_person;
    extern const float resource_per_person;
    extern const float births_per_person;
    extern const float deaths_per_person;
    extern const float industry_growth_cap;

    struct development{
      static std::vector<std::vector<float> > ship_cost;
      static std::vector<float> ship_buildtime;

      std::vector<float> fleet_growth; // accumulated ships per type
      std::vector<float> new_research; // accumulated research per field
      std::vector<float> industry; // population that can be allocated per branch
      std::vector<float> resource; // resources in storage per type
    };

    struct choice_t{
      std::vector<float> population; // proportion to allocate per sector
      development dev;

      choice_t();
      void normalize();
    };

    struct solar{

      // evolution data
      development dev;
      float population_number; // total population
      float population_happy; // proportion happy
      float defense_current; // ~number of ships
      float defense_capacity;
      std::vector<float> resource; // non-mined resources in solar
      std::set<idtype> ships;

      // technical data
      point position;
      sint owner;
      sfloat radius;

      float resource_constraint(std::vector<float> r); // how many r can be used before resource runs out?

      solar();
    };
  };
};
#endif
