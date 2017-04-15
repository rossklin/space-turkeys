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
    // base upgrades for base fleet interactions
    for (auto a : fleet::all_interactions()) data[a] = compile_upgrade(a);
  }
  
  return data;
}
