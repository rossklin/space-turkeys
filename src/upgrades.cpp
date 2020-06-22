#include "upgrades.h"

#include <rapidjson/document.h>

#include <list>
#include <string>

#include "fleet.h"
#include "interaction.h"
#include "solar.h"
#include "utility.h"

using namespace std;
using namespace st3;

const hm_t<string, upgrade> &upgrade::table() {
  static bool init = false;
  static hm_t<string, upgrade> data;

  if (init) return data;

  auto pdoc = utility::get_json("upgrade");
  auto &doc = (*pdoc)["upgrade"];

  auto get_multiplier = [](string v) -> float {
    size_t split = v.find('#');
    return stof(v.substr(0, split));
  };

  auto get_constant = [](string v) -> float {
    size_t split = v.find('#');
    return stof(v.substr(split + 1));
  };

  upgrade u, a;
  for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); i++) {
    a = u;
    string name = i->name.GetString();
    for (auto j = i->value.MemberBegin(); j != i->value.MemberEnd(); j++) {
      bool success = false;
      string stat_name = j->name.GetString();
      if (j->value.IsString()) {
        string stat_value = j->value.GetString();
        success = a.modify.parse(stat_name, stat_value);
      } else if (stat_name == "interactions") {
        for (auto k = j->value.Begin(); k != j->value.End(); k++) {
          a.inter.insert(k->GetString());
          success = true;
        }
      } else if (stat_name == "hook") {
        for (auto k = j->value.MemberBegin(); k != j->value.MemberEnd(); k++) {
          string hook_name = k->name.GetString();
          for (auto x = k->value.Begin(); x != k->value.End(); x++) {
            a.hook[hook_name].insert(x->GetString());
            success = true;
          }
        }
      }

      if (!success) {
        throw parse_error("Invalid upgrade stat for " + name + ": " + stat_name);
      }
    }

    data[name] = a;
  }

  delete pdoc;
  init = true;
  return data;
}
