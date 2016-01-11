#include <cstring>
#include "cost.h"

using namespace std;
using namespace st3;
using namespace cost;

sector_allocation<sector_cost> cost::sector_expansion;
ship_allocation<resource_base> cost::ship_build;

void st3::cost::initialize(){
  // sector costs
  // initialize to zero
  memset(&sector_expansion, 0, sizeof(sector_expansion));
  
  // T research;
  sector_expansion.research.res.organics = 1;
  sector_expansion.research.res.gases = 1;
  sector_expansion.research.res.metals = 1;
  sector_expansion.research.water = 1;
  sector_expansion.research.space = 1;
  sector_expansion.research.time = 10;
  
  // T culture;
  sector_expansion.culture.res.organics = 8;
  sector_expansion.culture.res.gases = 4;
  sector_expansion.culture.res.metals = 5;
  sector_expansion.culture.water = 10;
  sector_expansion.culture.space = 10;
  sector_expansion.culture.time = 4;

  // T military;
  sector_expansion.military.res.organics = 0;
  sector_expansion.military.res.gases = 4;
  sector_expansion.military.res.metals = 8;
  sector_expansion.military.water = 2;
  sector_expansion.military.space = 4;
  sector_expansion.military.time = 6;

  // T mining;
  sector_expansion.mining.res.organics = 0;
  sector_expansion.mining.res.gases = 8;
  sector_expansion.mining.res.metals = 4;
  sector_expansion.mining.water = 4;
  sector_expansion.mining.space = 4;
  sector_expansion.mining.time = 6;

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
}
