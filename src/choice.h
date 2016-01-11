#ifndef _STK_CHOICE
#define _STK_CHOICE

#include <list>

#include "types.h"
#include "command.h"
#include "solar.h"

namespace st3{
  /*! struct representing what a player can chose */
  namespace choice{
    struct c_research{
      static const int max_allocation = 10;

      std::string identifier;
    };

    struct c_military{
      static const int max_allocation = 12;

      cost::ship_allocation<int> ship_priority;
      cost::turret_allocation<int> turret_priority;
    };

    struct c_mining : public cost::resource_allocation<int>{
      static const int max_allocation = 9;
    };
    
    struct c_expansion : public cost::sector_allocation<int>{
      static const int max_allocation = 12;
    };
        
    /*! choice of priorities for solar system */
    struct c_solar{
      static const int max_allocation = 10;
      
      cost::sector_allocation<int> allocation;
      c_military military;
      c_mining mining;
      c_expansion expansion;

      c_solar();
      int count_allocation();
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
