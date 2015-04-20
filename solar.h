#ifndef _STK_SOLAR
#define _STK_SOLAR

#include <list>
#include <vector>
#include "types.h"

namespace st3{
  struct research_t;

  struct solar_choice{
    enum i_index{
      i_infrastructure = 0,
      i_agriculture,
      i_ship,
      i_research,
      i_resource,
      i_num
    };
    enum o_index{
      o_metal = 0,
      o_gas,
      o_num
    };
    enum p_index{
      p_research = 0,
      p_industry,
      p_resource,
      p_num
    };

    std::vector<float> p;
    std::vector<float> research;
    std::vector<float> industry;
    std::vector<float> resource;
    hm_t<shipclass_t, float> industry_fleet;

    void normalize();
  };

  struct solar{
    static idtype id_counter;

    // evolution data
    hm_t<shipclass_t, float> fleet_growth;
    hm_t<shipclass_t, std::list<ship> > fleet;
    std::vector<float> research;
    float population_number;
    float population_happy;
    std::vector<float> industry;
    std::vector<float> resource;
    std::vector<float> resource_storage;

    // technical data
    point position;
    sint owner;
    sfloat radius;

    void tick(solar_choice c, float dt, research_t &r);
    float resource_constraint(std::vector<float> r); // how many r can be used before resource runs out?
  };
};
#endif
