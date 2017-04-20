#include "command.h"

using namespace std;
using namespace st3;

st3::command::command(){
  source = "";
  target = "";
}

bool st3::operator ==(const command &a, const command &b){
  bool res = a.source == b.source && a.target == b.target && a.action == b.action;
  return res;
}
