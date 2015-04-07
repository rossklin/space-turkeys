#include "command.h"

st3::command::command(){
  source = "";
  target = "";
  quantity = -1;
  locked = false;
  lock_qtty = -1;
}

bool st3::operator ==(const command &a, const command &b){
  return !(a.source.compare(b.source) || a.target.compare(b.target));
}
