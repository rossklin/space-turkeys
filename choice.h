#ifndef _STK_CHOICE
#define _STK_CHOICE

#include <list>
#include <vector>

#include "types.h"
#include "command.h"

namespace st3{

  struct alter_command{
    bool delete_c;
    bool lock;
    bool unlock;
    int increment;

    alter_command();
  };

  struct choice{
    std::list<command> commands;
  };
};
#endif
