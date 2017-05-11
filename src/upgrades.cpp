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

const hm_t<string, upgrade> &upgrade::table(){
  static bool init = false;
  static hm_t<string, upgrade> data;

  if (init) return data;
    
  auto pdoc = utility::get_json("upgrade");
  auto &doc = (*pdoc)["upgrade"];

  upgrade u, a;
  for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); i++) {
    a = u;
    string name = i -> name.GetString();
    if (i -> value.HasMember("speed")) a.modify.speed = i -> value["speed"].GetDouble();
    if (i -> value.HasMember("vision")) a.modify.vision_range = i -> value["vision"].GetDouble();
    if (i -> value.HasMember("hp")) a.modify.hp = i -> value["hp"].GetDouble();
    if (i -> value.HasMember("ship_damage")) a.modify.ship_damage = i -> value["ship_damage"].GetDouble();
    if (i -> value.HasMember("solar_damage")) a.modify.solar_damage = i -> value["solar_damage"].GetDouble();
    if (i -> value.HasMember("accuracy")) a.modify.accuracy = i -> value["accuracy"].GetDouble();
    if (i -> value.HasMember("interaction_radius")) a.modify.interaction_radius_value = i -> value["interaction_radius"].GetDouble();
    if (i -> value.HasMember("load_time")) a.modify.load_time = i -> value["load_time"].GetDouble();

    if (i -> value.HasMember("interactions")){
      if (!i -> value["interactions"].IsArray()) {
	throw runtime_error("Error: loading upgrades: interactions is not an array!");
      }

      auto &interactions = i -> value["interactions"];
      for (auto j = interactions.Begin(); j != interactions.End(); j++) a.inter.insert(j -> GetString());
    }

    data[name] = a;
  }

  delete pdoc;
  init = true;
  return data;
}
