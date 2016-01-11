#ifndef _STK_CHOICE
#define _STK_CHOICE

#include <list>

#include "types.h"
#include "command.h"
#include "solar.h"

namespace st3{
  /*! struct representing what a player can chose */
  namespace choice{ 
    const int max_allocation = 10;
    
    struct c_research{
      std::string identifier;
    };

    struct c_military{
      cost::ship_allocation<sfloat> ship_priority;
      cost::turret_allocation<sfloat> turret_priority;
    };

    struct c_mining : public cost::resource_allocation<sfloat>{
    };
    
    struct c_expansion : public cost::sector_allocation<sfloat>{
    };
        
    /*! choice of priorities for solar system */
    struct c_solar{
      cost::sector_allocation<sfloat> allocation;
      c_military military;
      c_mining mining;
      c_expansion expansion;

      c_solar();
      void normalize(); // normalize to proportions
      sfloat count_allocation();
    };

    struct choice{
      c_research research;
      hm_t<source_t, std::list<command> > commands; /*!< table of commands for game entities */
      hm_t<idtype, c_solar> solar_choices; /*!< table of choices for solar system evolution */ 
      hm_t<source_t, waypoint> waypoints; /*!< table of generated waypoints */
    };
  };
};
#endif
