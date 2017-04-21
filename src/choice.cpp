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

hm_t<string,c_solar> const &choice::c_solar::template_table(){
  static hm_t<string, c_solar> data;
  static bool first = true;
  using namespace cost;

  if (first){
    first = false;

    // basic pop growth
    c_solar empty;
    c_solar x = empty;
    x.allocation[keywords::key_culture] = 1;
    data["basic growth"] = x;

    // culture growth
    x = empty;
    x.allocation[keywords::key_culture] = 1;
    x.allocation[keywords::key_expansion] = 1;
    x.allocation[keywords::key_mining] = 1;
    
    x.expansion[keywords::key_culture] = 1;

    x.mining[keywords::key_organics] = 2;
    x.mining[keywords::key_gases] = 1;
    x.mining[keywords::key_metals] = 1;

    data["culture growth"] = x;

    x = empty;
    x.allocation[keywords::key_culture] = 1;
    x.allocation[keywords::key_expansion] = 1;
    x.allocation[keywords::key_mining] = 1;
    x.allocation[keywords::key_military] = 3;

    x.mining[keywords::key_gases] = 1;
    x.mining[keywords::key_metals] = 1;
    x.mining[keywords::key_organics] = 1;
    
    x.expansion[keywords::key_culture] = 1;
    x.expansion[keywords::key_mining] = 2;
    x.expansion[keywords::key_military] = 3;
    
    x.military.c_ship[keywords::key_scout] = 1;
    x.military.c_ship[keywords::key_fighter] = 2;
    x.military.c_turret[keywords::key_radar_turret] = 1;
    x.military.c_turret[keywords::key_rocket_turret] = 2;

    data["military expansion"] = x;
  }

  return data;
}
