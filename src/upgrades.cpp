#include <list>
#include <string>

#include "interaction.h"
#include "upgrades.h"
#include "fleet.h"
#include "utility.h"
#include "solar.h"

using namespace std;
using namespace st3;

upgrade compile_upgrade(string i) {
  upgrade u;
  u.inter.insert(i);
  return u;
}

hm_t<string, upgrade> &upgrade::table(){
  static bool init = false;
  static hm_t<string, upgrade> data;

  if (!init){
    init = true;
    // upgrades for base fleet interactions
    for (auto a : interaction::table()) data[a.first] = compile_upgrade(a.first);

    // tech upgrades
    data["ship armor"].modify.hp = 1;
    data["ship speed"].modify.speed = 1;
    data["ship weapons"].modify.ship_damage = 1;
    data["ship weapons"].modify.solar_damage = 1;
  }
  
  return data;
}
