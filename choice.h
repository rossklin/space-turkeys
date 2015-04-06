#ifndef _STK_CHOICE
#define _STK_CHOICE

#include <list>
#include <vector>

#include "types.h"

namespace st3{
  struct command{
    // game data
    idtype source;
    target_t target;
    sint quantity;

    // interface control variables
    sbool locked;
    sint lock_qtty;
    sbool selected;

    command();
  };

  struct alter_command{
    bool delete_c;
    bool lock;
    bool unlock;
    int increment;

    alter_command();
  };

  struct choice{
    static int comid;
    // data
    hm_t<idtype, command> solar_commands;
    hm_t<idtype, command> fleet_commands;
      
    // interface routines
    void alter_selected(alter_command c);
    void clear_selection();
  };
};
#endif
