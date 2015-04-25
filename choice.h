#ifndef _STK_CHOICE
#define _STK_CHOICE

#include <list>

#include "types.h"
#include "command.h"
#include "solar.h"

namespace st3{
  struct choice{
    hm_t<source_t, std::list<command> > commands;
    hm_t<idtype, solar::choice_t> solar_choices;
  };
};
#endif
