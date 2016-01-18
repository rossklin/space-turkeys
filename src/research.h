#ifndef _STK_RESEARCH
#define _STK_RESEARCH

#include <string>
#include "ship.h"
#include "turret.h"
#include "solar.h"

namespace st3{
  namespace research{
    extern cost::ship_allocation<ship> &ship_templates;
    extern cost::turret_allocation<turret> &turret_templates;

    void initialize();
    void cleanup();
    
    /*! struct representing the research level of a player */
    struct data{
      std::string x;

      /*! default constructor */
      data();

      void develope(float x);
      ship build_ship(std::string v);
      turret build_turret(std::string v);
      void colonize(solar::solar *s);

      // todo: move this to a ship sub class or ship data
      int colonizer_population();
    };    
  };
};
#endif
