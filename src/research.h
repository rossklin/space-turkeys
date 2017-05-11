#ifndef _STK_RESEARCH
#define _STK_RESEARCH

#include <string>
#include <set>
#include <list>

#include "ship.h"
#include "solar.h"
#include "choice.h"

namespace st3{
  namespace research{
    const std::string upgrade_all_ships = "upgrade all ships";

    struct tech {
      std::string name;

      // requirements
      sfloat cost;
      sint req_facility_level;
      std::set<std::string> depends;

      //effects
      hm_t<std::string, std::set<std::string> > ship_upgrades;
    };
    
    /*! struct representing the research level of a player */
    struct data {
      static const hm_t<std::string, tech> &get_tree();
      std::set<std::string> researched;
      sfloat accumulated;
      sint facility_level;

      std::list<std::string> available();
      ship build_ship(std::string v, solar::ptr sol);
      void repair_ship(ship &s, solar::ptr sol);
      bool can_build_ship(std::string v, solar::ptr s);
      hm_t<std::string, choice::c_solar> solar_template_table(solar::ptr s);
    };    
  };
};
#endif
