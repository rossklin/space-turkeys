#include "choice.h"
#include "ship.h"

using namespace std;
using namespace st3;
using namespace choice;

c_solar::c_solar(){
  set_zeros();
}

c_solar c_solar::set_zeros(){  
  for (auto v : keywords::sector) allocation[v] = 0;
  for (auto v : keywords::resource) mining[v] = 0;
  return *this;
}

c_solar choice::c_solar::normalize(){
  allocation.normalize();
  mining.normalize();
  return *this;
}

bool choice::c_solar::do_develop() {
  return building_queue.size() && building_queue.front() != keywords::build_disabled;
}

bool choice::c_solar::do_produce() {
  return ship_queue.size() && ship_queue.front() != keywords::build_disabled;
}
