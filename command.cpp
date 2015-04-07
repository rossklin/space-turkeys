#include "command.h"

st3::command::command(){
  source.id = -1;
  source.type = source_t::SOLAR;
  target.id = -1;
  target.type = target_t::SOLAR;
  quantity = -1;
  locked = false;
  lock_qtty = -1;
}
