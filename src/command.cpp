#include "command.hpp"

#include "fleet.hpp"

using namespace std;
using namespace st3;

st3::command::command() {
  source = "";
  target = "";
  action = "";
  origin = "";
  policy = fleet::policy_maintain_course;
}

bool st3::operator==(const command &a, const command &b) {
  bool res = a.source == b.source && a.target == b.target && a.action == b.action;
  return res;
}
