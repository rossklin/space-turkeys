#ifndef _STK_COMMAND
#define _STK_COMMAND

namespace st3{
  struct command{
    // game data
    sint source;
    target_t target;
    sint quantity;
    sint signal_delay;
    std::list<point> path;

    // interface control variables
    sint is_locked;
    sint lock_qtty;
    sint is_selected;

    command();
  };
};

#endif
