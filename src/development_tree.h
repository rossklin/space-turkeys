#ifndef _STK_DEVELOPMENT_TREE
#define _STK_DEVELOPMENT_TREE

#include <set>
#include <string>
#include <rapidjson/document.h>

#include "types.h"
#include "cost.h"

namespace st3 {
  namespace development {
    class node {
    public:
      std::string name;
      hm_t<std::string, sfloat> sector_boost;
      hm_t<std::string, std::set<std::string> > ship_upgrades;

      // requirements
      sfloat cost_time;
      hm_t<std::string, sint> depends_facilities;
      std::set<std::string> depends_techs;

      node();
      virtual void read_from_json(const rapidjson::GenericValue &v);
    };

    template<typename T>
    hm_t<std::string, T> read_from_json(const rapidjson::GenericValue &v, T init);
  };
};

#endif
