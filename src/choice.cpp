#include "choice.h"

using namespace std;
using namespace st3;
using namespace choice;

c_solar::c_solar(){
  set_zeros();
}

void c_solar::set_zeros(){  
  for (auto v : cost::keywords::sector)
    allocation[v] = 0;

  for (auto v : cost::keywords::ship)
    military.c_ship[v] = 0;

  for (auto v : cost::keywords::turret)
    military.c_turret[v] = 0;

  for (auto v : cost::keywords::resource)
    mining[v] = 0;

  for (auto v : cost::keywords::expansion)
    expansion[v] = 0;
}

void choice::c_solar::normalize(){
  allocation.normalize();
  
  float sum = military.c_ship.count() + military.c_turret.count();
  if (sum <= 0){
    for (auto v : cost::keywords::ship)
      military.c_ship[v] = 1;
    for (auto v : cost::keywords::turret)
      military.c_turret[v] = 1;
    sum = military.c_ship.count() + military.c_turret.count();
  }
  
  for (auto v : cost::keywords::ship)
    military.c_ship[v] /= sum;
  for (auto v : cost::keywords::turret)
    military.c_turret[v] /= sum;
  
  mining.normalize();
  expansion.normalize();
}
