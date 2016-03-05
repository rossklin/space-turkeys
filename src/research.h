#ifndef _STK_RESEARCH
#define _STK_RESEARCH

#include <string>
#include "ship.h"
#include "turret.h"
#include "solar.h"

namespace st3{
  namespace research{
    ship ship_template(std::string k);
    turret turret_template(std::string k);
    
    /*! struct representing the research level of a player */
    struct data{
      std::string x;

      /*! default constructor */
      data();

      void develope(float x);
      ship::ptr build_ship(std::string v);
      turret build_turret(std::string v);
      void colonize(solar::ptr s);

      // todo: move this to a ship sub class or ship data
      int colonizer_population();
    };    
  };
};
#endif
