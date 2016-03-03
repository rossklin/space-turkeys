#ifndef _STK_CHOICE
#define _STK_CHOICE

#include <list>

#include "types.h"
#include "command.h"
#include "waypoint.h"

namespace st3{
  /*! struct representing what a player can chose */
  namespace choice{ 
    const int max_allocation = 10;
    
    struct c_research{
      std::string identifier;
    };

    struct c_military{
      cost::countable_ship_allocation<sfloat> c_ship;
      cost::countable_turret_allocation<sfloat> c_turret;
    };

    typedef cost::countable_resource_allocation<sfloat> c_mining;
    typedef cost::countable_sector_allocation<sfloat> c_expansion;

    /*! choice of priorities for solar system */
    struct c_solar{
      cost::countable_sector_allocation<sfloat> allocation;

      c_military military;
      c_mining mining;
      c_expansion expansion;

      c_solar();
      void normalize(); // normalize to proportions
      void set_zeros();
      static hm_t<std::string,c_solar> const &template_table();
    };


    struct choice{
      c_research research;
      hm_t<combid, std::list<command> > commands; /*!< table of commands for game entities */
      hm_t<combid, c_solar> solar_choices; /*!< table of choices for solar system evolution */ 
      hm_t<combid, waypoint> waypoints; /*!< table of generated waypoints */
    };
  };
};
#endif
