#ifndef _STK_SHIP
#define _STK_SHIP

#include <string>
#include <vector>

#include "types.h"

namespace st3{
  struct research;

  struct ship{
    static idtype id_counter;
    typedef std::string class_t;
    static class_t class_scout;
    static class_t class_fighter;
    static std::vector<class_t> classes;
    static hm_t<class_t, std::vector<float> > resource_cost;
    
    idtype fleet_id;
    class_t ship_class;
    point position;
    sfloat angle;
    sfloat speed;
    sint owner;
    sint hp;
    sbool was_killed;
    sfloat interaction_radius;

    ship();
    ship(class_t c, research &r);
  };
};
#endif
