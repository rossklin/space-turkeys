#pragma once

#include <string>
#include <set>
#include <list>

#include "ship.h"
#include "solar.h"
#include "choice.h"
#include "development_tree.h"

namespace st3{
  namespace research{
    const std::string upgrade_all_ships = "upgrade all ships";
    class tech : public development::node {
    public:
      tech();
      tech(const tech &f);
      void read_from_json(const rapidjson::Value &v);
    };
    
    /*! struct representing the research level of a player */
    struct data {
      static const hm_t<std::string, tech> &table();
      hm_t<std::string, tech> tech_map;
      std::string researching;
      hm_t<std::string, sint> facility_level;

      data();
      static std::set<std::string> get_tech_upgrades(std::string sc, std::string t);
      tech &access(std::string v);
      std::list<std::string> list_tech_requirements(std::string v) const;
      std::list<std::string> available() const;
      std::set<std::string> researched() const;
      ship build_ship(idtype id, std::string v, solar::ptr sol) const;
      void repair_ship(ship &s, solar::ptr sol) const;
      bool can_build_ship(std::string v, solar::ptr s, std::list<std::string> *data = 0) const;
      float solar_modifier(std::string v) const;
    };    
  };
};
