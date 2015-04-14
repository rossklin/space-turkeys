#ifndef _STK_CHOICE
#define _STK_CHOICE

#include <list>

#include "types.h"
#include "command.h"

namespace st3{
  struct choice{
    hm_t<source_t, std::list<command> > commands;
  };
};
#endif
