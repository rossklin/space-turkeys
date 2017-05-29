#ifndef _STK_RESEARCH
#define _STK_RESEARCH

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
      std::set<std::string> researched;
      sfloat accumulated;
      std::string researching;
      hm_t<std::string, sint> facility_level;

      data();
      std::list<std::string> list_tech_requirements(std::string v);
      std::list<std::string> available();
      ship build_ship(std::string v, solar::ptr sol);
      void repair_ship(ship &s, solar::ptr sol);
      bool can_build_ship(std::string v, solar::ptr s, std::list<std::string> *data = 0);
      hm_t<std::string, choice::c_solar> solar_template_table(solar::ptr s);
    };    
  };
};
#endif
