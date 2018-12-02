#ifndef _STK_CHOICE
#define _STK_CHOICE

#include <list>

#include "types.h"
#include "command.h"
#include "waypoint.h"
#include "fleet.h"
#include "cost.h"

namespace st3{
  /*! struct representing what a player can chose */
  namespace choice{ 
    const int max_allocation = 10;

    /*! choice of priorities for solar system */
    struct c_solar{
      std::list<std::string> building_queue;
      std::list<std::string> ship_queue;
      bool do_develop();
      bool do_produce();
    };

    struct choice{
      std::string research;
      cost::ship_allocation military;
      hm_t<combid, std::list<command> > commands; /*!< table of commands for game entities */
      hm_t<combid, c_solar> solar_choices; /*!< table of choices for solar system evolution */ 
      hm_t<combid, waypoint> waypoints; /*!< table of generated waypoints */
      hm_t<combid, fleet> fleets;
    };
  };
};
#endif
