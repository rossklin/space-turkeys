#include "command.h"

st3::command::command(){
  source = "";
  target = "";
}

bool st3::operator ==(const command &a, const command &b){
  return !(a.source.compare(b.source) || a.target.compare(b.target));
}
