#ifndef _STK_SHIP_STATS
#define _STK_SHIP_STATS

#include <string>
#include <set>

#include "types.h"
#include "game_object.h"
#include "interaction.h"
#include "cost.h"

namespace st3{
  struct ship_stats_modifier {
    float a;
    float b;

    ship_stats_modifier();
    void parse(std::string v);
    float apply(float x) const;
    void combine(const ship_stats_modifier &b);
  };

  namespace sskey {
    enum key {
      thrust, hp, mass, accuracy, evasion, ship_damage, solar_damage, interaction_radius, vision_range, load_time, cargo_capacity, build_time, regeneration, shield, detection, stealth, cannon_flex, count
    };
  };

  template<typename T>
  class modifiable_ship_stats {
  public:
    static sskey::key lookup_key(std::string name);

    std::vector<T> stats;

    modifiable_ship_stats();
    modifiable_ship_stats(const modifiable_ship_stats &s);
    virtual ~modifiable_ship_stats() = default;
  };

  class ssmod_t : public modifiable_ship_stats<ship_stats_modifier> {
  public:
    ssmod_t();
    ssmod_t(const ssmod_t &s);
    void combine(const ssmod_t &b);
    bool parse(std::string key, std::string value);
  };

  class ssfloat_t : public modifiable_ship_stats<sfloat> {
  public:
    ssfloat_t();
    ssfloat_t(const ssfloat_t &s);
    bool insert(std::string key, sfloat value);
    float get_dps();
    float get_hp();
    float get_strength();
    void modify_with(const ssmod_t &m);
  };
    
  class ship_stats : public ssfloat_t {
  public:
    static const hm_t<std::string, ship_stats>& table();
    // ship class info
    class_t ship_class; /*!< ship class */
    std::set<std::string> tags;
    std::set<std::string> upgrades;

    // cost and req
    std::string depends_tech;
    sint depends_facility_level;
    cost::res_t build_cost;
    sfloat build_time;

    // graphics
    std::vector<std::pair<point, unsigned char> > shape;

    ship_stats();
    ship_stats(const ship_stats &s);
  };
};
#endif
