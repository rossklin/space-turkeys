#ifndef _STK_SOLAR
#define _STK_SOLAR

#include <set>
#include <vector>
#include "types.h"
#include "ship.h"

namespace st3{
  struct research;

  /*! types and functions related to solars 
    
    When adding [sub]sectors, remember to:
    - add initializers in solar::initialize
    - add the [sub]sector in solar::sub_increment
    - add the [sub]sector in solar gui templates
   */
  namespace solar{
    extern idtype id_counter;

    /*! work sectors to which population can be allocated */
    enum d_work{
      work_expansion = 0,
      work_ship,
      work_research,
      work_resource,
      work_defense,
      work_num
    };

    /*! types of resource which resource workers can mine */
    enum d_resource{
      resource_metal = 0,
      resource_gas,
      resource_organics,
      resource_num
    };

    /*! ship classes which ship workers can build */
    enum d_ship{
      ship_scout = 0,
      ship_fighter,
      ship_bomber,
      ship_colonizer,
      ship_num
    };

    /*! defense work choices */
    enum d_defense{
      defense_enhance = 0,
      defense_repair,
      defense_num
    };

    /*! solar system coefficient: births per person per unit time */
    extern const float births_per_person;

    /*! solar system coefficient: deaths per person per unit time */
    extern const float deaths_per_person;

    /*! solar system coefficient: limit of food production boost factor per person */
    extern const float agriculture_limit_coefficient;

    /*! base food production per farmer */
    extern const float food_per_person;

    /*! food production per farmer increase per research level */
    extern const float fpp_per_research;

    /*! solar system coefficient: effect factor for food */
    extern const float feed_boost_coefficient;

    /*! solar system coefficient: effect factor for food */
    extern const float crowding_rate;

    /*! number of people that go in a colonizer */
    extern const float colonizer_population;

    /*! resource costs for each resource for each ship class */
    extern std::vector<std::vector<float> > ship_cost;

    /*! resource costs for each resource for each industry sector */
    extern std::vector<std::vector<float> > industry_cost;
      
    /*! ship construction units required to build each ship class */
    extern std::vector<float> ship_buildtime;

    /*! names of work sectors in enum d_work */
    extern std::vector<std::string> work_names;

    /*! names of work subsectors */
    extern std::vector<std::vector<std::string> > sub_names;

    /*! production per person in work sectors */
    extern std::vector<float> p3;

    /*! choice of priorities for solar system */
    struct choice_t{
      /*! proportion of people to work in sectors (non-farmers) */
      float workers;

      /*! proportion of workers in each sector */
      std::vector<sfloat> sector; 

      /*! proportions of workers in each subsector */
      std::vector<std::vector<sfloat> > subsector;

      /*! normalize each sector and subsector */
      void normalize();      

      /*! default constructor with equal proportions */
      choice_t();
    };

    /*! data representing a solar system */
    struct solar{
      /*! work places available in main sectors */
      std::vector<sfloat> industry; 

      /*! ship growth per class */
      std::vector<sfloat> fleet_growth;

      /*! amount of research produced per field */
      std::vector<sfloat> new_research;

      /*! amount of each resource in storage */
      std::vector<sfloat> resource_storage;

      /*! number of inhabitants */
      sfloat population_number;

      /*! proportion of happy inhabitants */
      sfloat population_happy;

      /*! current defense level */
      sfloat defense_current;
      
      /*! defense level capacity */
      sfloat defense_capacity;

      /*! available area to be shared by industry and agriculture */
      sfloat usable_area;

      /*! vision radius */
      sfloat vision;

      /*! resources available for mining */
      std::vector<sfloat> resource;

      /*! ships landed at solar */
      std::set<idtype> ships;

      /*! position of solar */
      point position;

      /*! id of the player that owns the solar */
      sint owner;

      /*! radius of the solar */
      sfloat radius;

      /*! default constructor (data are set in game_data::build) */
      solar();
      
      /*! compute how many units of a given resource cost can be built
	@param r resource cost per resource type
	@return how many r there are stored resources for
      */
      float resource_constraint(std::vector<float> r);

      /*! compile a string describing the solar
	@return the string 
      */
      std::string get_info();

      /*! compute the increment per unit time in a subsector
	@param r current research level
	@param sector_idx the work sector
	@param sub_idx the sub sector
	@param n the number of assigned workers
	@return increment per unit time
      */
      float sub_increment(research const &r, int sector_idx, int sub_idx, float n);

      /*! compute the population increment per unit time 
	@param r current research level
	@param n number of assigned farmers
	@return increment per unit time
      */
      float pop_increment(research const &r, float n);
    };

    /*! initialize static data */
    void initialize();
  };
};
#endif
