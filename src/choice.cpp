#include "choice.h"

#include "ship.h"

using namespace std;
using namespace st3;
using namespace choice;

bool choice::c_solar::do_develop() {
  return building_queue.size() && building_queue.front() != keywords::build_disabled;
}

bool choice::c_solar::do_produce() {
  return ship_queue.size() && ship_queue.front() != keywords::build_disabled;
}

string choice::c_solar::devstring() {
  return building_queue.size() ? building_queue.front() : keywords::build_disabled;
}
