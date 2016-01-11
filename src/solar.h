#ifndef _STK_SOLAR
#define _STK_SOLAR

#include <set>
#include <vector>

#include "types.h"
#include "ship.h"

namespace st3{
  struct research;

  /*! types and functions related to solars */
  namespace solar{
    extern idtype id_counter;

    /*! data representing a solar system */
    struct solar{
      /*! ship growth per class */
      cost::ship_base fleet_growth;

      /*! amount of research produced */
      sfloat research;

      /*! nr of homes available to live in */
      sfloat housing;
      sfloat water;
      sfloat space;
      
      /*! amount of each resource */
      cost::resource_allocation<cost::resource_data> resource;

      /*! development of each sector */
      cost::sector_base sector;

      /*! ecosystem status */
      sfloat ecology;

      /*! number of inhabitants */
      sfloat population;

      /*! proportion of happy inhabitants */
      sfloat happiness;

      /*! current defense level */
      sfloat defense_current;
      
      /*! defense level capacity */
      sfloat defense_capacity;

      /*! vision radius */
      sfloat vision;

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
	@param r resource cost per resource type
	@return how many r there are stored resources for
      */
      float resource_constraint(cost::resource_base r);

      /*! compile a string describing the solar
	@return the string 
      */
      std::string get_info();

      /*! compute the population increment per unit time 
	@param r current research level
	@param n number of assigned farmers
	@return increment per unit time
      */
      float pop_increment(research const &r, float n);

      /*! compute number of farmers required for given relative growth
	@param q requested relative growth
	@param r current research state
	@return required number of farmers
      */
      float farmers_for_growthratio(float q, research const &r);
    };
  };
};
#endif
