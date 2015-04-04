#ifndef _STK_CHOICE
#define _STK_CHOICE

#include <list>
#include <vector>

#include "types.h"

namespace st3{
  struct command{
    // game data
    target_t target;
    sint quantity;

    // interface control variables
    sbool locked;
    sint lock_qtty;
    sbool selected;
  };

  struct alter_command{
    bool delete_c;
    bool lock;
    bool unlock;
    int increment;

    alter_command();
  };

  struct choice{
    // data
    hm_t<idtype, command> fleet_commands;
    hm_t<idtype, command> solar_commands;

    // interface routines
    void alter_selected(alter_command c);
    void clear_selection();
  };
};
#endif
