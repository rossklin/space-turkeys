#ifndef _STK_PLANET
#define _STK_PLANET

#include <vector>
#include <list>

#include "command.h"

namespace st3{
  struct STK_Planet{
    list<command> commands;

    sint shutdown;
    sint inactive;
    sint x;
    sint y;
    sfloat radius;
    sfloat productivity;
    sfloat population;
    sint owner;
    sint id;
    sfloat research;
    sfloat resource;

    bool operator == (STK_Planet a);
    void give_command(STK_Command c);
    void increment();
  };
};
#endif
