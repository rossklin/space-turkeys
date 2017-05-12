#include "choice.h"
#include "ship.h"

using namespace std;
using namespace st3;
using namespace choice;

c_solar::c_solar(){
  set_zeros();
}

void c_solar::set_zeros(){  
  for (auto v : keywords::sector) allocation[v] = 0;
  for (auto v : ship::all_classes()) military[v] = 0;
  for (auto v : keywords::resource) mining[v] = 0;
  development = "";
}

void choice::c_solar::normalize(){
  allocation.normalize();
  military.normalize();
  mining.normalize();
}
