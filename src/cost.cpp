#include "cost.h"

using namespace std;
using namespace st3;
using namespace cost;

sector_allocation<sector_cost> cost::sector_expansion;
sector_cost cost::housing;
ship_allocation<resource_base> cost::ship_build;

void st3::cost::initialize(){
  // sector costs
  sector_expansion.agriculture.res.organics = 1;
  sector_expansion.agriculture.water = 1;
  sector_expansion.agriculture.space = 2;

  sector_expansion.infrastructure.res.metals = 1;
  sector_expansion.infrastructure.res.gases = 1;

  ...

  // ship costs
  ship_build.scout.res.metals = 1;
  ship_build.scout.res.gases = 1;
  ship_biuld.scout.time = 1;

  ship_build.fighter.res.metals = 2;
  ship_build.fighter.res.gases = 1;
  ship_build.fighter.time = 2;

  ship_build.bomber.res.metals = 4;
  ship_build.bomber.res.gases = 3;
  ship_build.bomber.time = 4;

  ship_build.colonizer.res.metals = 4;
  ship_build.colonizer.res.gases = 2;
  ship_build.colonizer.res.organics = 3;
  ship_build.colonizer.time = 6;

  // housing costs
  housing.res.metals = 1;
  hosing.water = 1;
  housing.space = 1;
}
