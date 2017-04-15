#include "command.h"

using namespace std;
using namespace st3;

st3::command::command(){
  source = "";
  target = "";
}

bool st3::operator ==(const command &a, const command &b){
  return !(a.source.compare(b.source) || a.target.compare(b.target) || a.action.compare(b.action));
}
