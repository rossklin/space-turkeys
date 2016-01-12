#ifndef _STK_SOLAR
#define _STK_SOLAR

#include <set>
#include <vector>

#include "types.h"
#include "ship.h"
#include "turret.h"

namespace st3{

  /*! types and functions related to solars */
  namespace solar{
    extern idtype id_counter;

    /*! data representing a solar system */
    struct solar{
      /*! ship growth per class */
      cost::ship_allocation<sfloat> fleet_growth;

      /*! turret growth per class */
      cost::turret_allocation<sfloat> turret_growth;

      /*! turrets in solar */
      std::list<turret> turrets;

      /*! amount of research produced */
      sfloat research;

      /*! nr of homes available to live in */
      sfloat housing;
      sfloat water;
      sfloat space;
      
      /*! amount of each resource */
      cost::resource_allocation<cost::resource_data> resource;

      /*! development of each sector */
      cost::sector_allocation<sfloat> sector;

      /*! ecosystem status */
      sfloat ecology;

      /*! number of inhabitants */
      sfloat population;

      /*! proportion of happy inhabitants */
      sfloat happiness;

      /*! ships landed at solar */
      std::set<idtype> ships;

      /*! position of solar */
      point position;

      /*! id of the player that owns the solar */
      sint owner;

      /*! radius of the solar */
      sfloat radius;

      /*! damage taken from different players */
      hm_t<idtype, float> damage_taken;

      /*! colonization attempts from different ships */
      hm_t<idtype, idtype> colonization_attempts;

      /*! default constructor (data are set in game_data::build) */
      solar();
      
      /*! compute how many units of a given resource cost can be built
          from resources in storage

	@param r resource cost per resource type
	@return how many r there are stored resources for
      */
      float resource_constraint(cost::resource_allocation<sfloat> r);

      /*! Compute degree of availability of space for natural habitat
          on scale [0,1] */
      float space_status();

      /*! Compute degree of availability of clean water on scale
          [0,1] */
      float water_status();

      /*! Compute the vision radius from turrets */
      float compute_vision();

      /*! Check whether any functioning turrets remain */
      bool has_defense();

      /*! compile a string describing the solar
	@return the string 
      */
      std::string get_info();
    };
  };
};
#endif
