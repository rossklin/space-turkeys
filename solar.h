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

    enum d_work{
      work_expansion = 0,
      work_ship,
      work_research,
      work_resource,
      work_num
    };

    enum d_resource{
      resource_metal = 0,
      resource_gas,
      resource_num
    };

    enum d_ship{
      ship_scout = 0,
      ship_fighter,
      ship_bomber,
      ship_num
    };

    extern const float births_per_person;
    extern const float deaths_per_person;
    extern const float agriculture_boost_coefficient;
    extern const float feed_boost_coefficient;

    struct development{
      static std::vector<std::vector<float> > ship_cost;
      static std::vector<float> ship_buildtime;
      static std::vector<std::string> work_names;
      static std::vector<std::vector<std::string> > sub_names;
      static std::vector<float> p3;
      static void initialize();

      // main work sectors
      std::vector<sfloat> industry; 

      // sub sectors
      std::vector<sfloat> fleet_growth;
      std::vector<sfloat> new_research;
      std::vector<sfloat> resource;
    };

    struct choice_t{
      float workers;
      void normalize();      
      choice_t();
      std::vector<sfloat> sector; 
      std::vector<std::vector<sfloat> > subsector;
    };


    struct solar{
      // evolution data
      development dev;
      sfloat population_number; // total population
      sfloat population_happy; // proportion happy
      sfloat defense_current; // ~number of ships
      sfloat defense_capacity;
      sfloat usable_area;
      sfloat vision;
      std::vector<sfloat> resource; // non-mined resources in solar
      std::set<idtype> ships;

      // technical data
      point position;
      sint owner;
      sfloat radius;

      solar();
      float resource_constraint(std::vector<float> r); // how many r can be used before resource runs out?
      std::string get_info();
      float sub_increment(research const &r, int sub_idx, int i, float n);
      float pop_increment(research const &r, float n);
    };

    void initialize();
  };
};
#endif
