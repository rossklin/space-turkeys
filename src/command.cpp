#include "command.h"

using namespace std;
using namespace st3;

const string command::action_waypoint = "waypoint";
const string command::action_follow = "follow";
const string command::action_join = "join";
const string command::action_attack = "attack";
const string command::action_land = "land";
const string command::action_colonize = "colonize";

st3::command::command(){
  source = "";
  target = "";
}

bool st3::operator ==(const command &a, const command &b){
  return !(a.source.compare(b.source) || a.target.compare(b.target) || a.action.compare(b.action));
}
