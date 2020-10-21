#pragma once

#include <list>

#include "command.hpp"
#include "types.hpp"

namespace st3 {
/*! struct representing what a player can chose */
const int max_allocation = 10;

/*! choice of priorities for solar system */
struct c_solar {
  std::list<std::string> building_queue;
  std::list<std::string> ship_queue;
};

struct choice {
  std::string research;
  hm_t<idtype, std::list<command> > commands; /*!< table of commands for game entities */
  hm_t<idtype, c_solar> solar_choices;        /*!< table of choices for solar system evolution */
  hm_t<idtype, waypoint_ptr> waypoints;       /*!< table of generated waypoints */
};
};  // namespace st3
