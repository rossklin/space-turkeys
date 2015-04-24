#ifndef _STK_SOLAR
#define _STK_SOLAR

#include <set>
#include <vector>
#include "types.h"
#include "ship.h"

namespace st3{
  struct research;

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
    std::vector<float> do_research;
    std::vector<float> industry;
    std::vector<float> resource;
    std::vector<float> industry_fleet;

    solar_choice();
    void normalize();
  };

  struct solar{
    static idtype id_counter;

    // evolution data
    std::vector<float> fleet_growth;
    std::set<idtype> ships;
    std::vector<float> new_research;
    float population_number;
    float population_happy;
    std::vector<float> industry;
    std::vector<float> resource;
    std::vector<float> resource_storage;

    // technical data
    point position;
    sint owner;
    sfloat radius;
    float defense_current;
    float defense_capacity;

    float resource_constraint(std::vector<float> r); // how many r can be used before resource runs out?
    void add_ship(ship s);

    solar();
  };
};
#endif
