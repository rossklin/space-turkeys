#ifndef _STK_CHOICE
#define _STK_CHOICE

#include <list>

#include "types.h"
#include "command.h"
#include "solar.h"

namespace st3{
  /*! struct representing what a player can chose */
  struct choice{
    hm_t<source_t, std::list<command> > commands; /*!< table of commands for game entities */
    hm_t<idtype, solar::choice_t> solar_choices; /*!< table of choices for solar system evolution */ 
    hm_t<source_t, waypoint> waypoints; /*!< table of generated waypoints */
  };
};
#endif
