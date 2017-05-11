#include <list>
#include <string>
#include <rapidjson/document.h>

#include "interaction.h"
#include "upgrades.h"
#include "fleet.h"
#include "utility.h"
#include "solar.h"

using namespace std;
using namespace st3;

hm_t<string, upgrade> &upgrade::table(){
  static bool init = false;
  static hm_t<string, upgrade> data;

  if (!init){
    init = true;
    
    auto doc = utility::get_json("upgrade");

    upgrade u, a;
    for (auto &x : doc){
      a = u;
      string name = x.name.GetString()
      if (x.value.HasMember("speed")) a.modify.speed = x.value["speed"].GetFloat();
      if (x.value.HasMember("vision")) a.modify.vision = x.value["vision"].GetFloat();
      if (x.value.HasMember("hp")) a.modify.hp = x.value["hp"].GetFloat();
      if (x.value.HasMember("ship_damage")) a.modify.ship_damage = x.value["ship_damage"].GetFloat();
      if (x.value.HasMember("solar_damage")) a.modify.solar_damage = x.value["solar_damage"].GetFloat();
      if (x.value.HasMember("accuracy")) a.modify.accuracy = x.value["accuracy"].GetFloat();
      if (x.value.HasMember("interaction_radius")) a.modify.interaction_radius = x.value["interaction_radius"].GetFloat();
      if (x.value.HasMember("load_time")) a.modify.load_time = x.value["load_time"].GetFloat();

      if (x.value.HasMember("interactions")){
	if (!x.value["interactions"].IsArray()) {
	  throw runtime_error("Error: loading upgrades: interactions is not an array!");
	}
	
	for (auto &i : x.value["interactions"]) a.inter.insert(i.GetString());
      }

      data[name] = a;
    }
  }
  
  return data;
}
