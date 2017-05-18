#ifndef _STK_SHIP
#define _STK_SHIP

#include <string>
#include <set>

#include "types.h"
#include "game_object.h"
#include "interaction.h"
#include "cost.h"

namespace st3{
  class game_data;
  class solar;

  struct ship_stats_modifier {
    float a;
    float b;

    ship_stats_modifier();
    void parse(std::string v);
    float apply(float x) const;
    void combine(const ship_stats_modifier &b);
  };

  template<typename T>
  class modifiable_ship_stats {
  public:
    enum key {
      speed, hp, mass, accuracy, evasion, ship_damage, solar_damage, interaction_radius_value, vision_range, load_time, cargo_capacity, depends_facility_level, build_time, regeneration, shield, detection, stealth, count
    };
    std::vector<T> stats;

    modifiable_ship_stats();
    modifiable_ship_stats(const modifiable_ship_stats &s);
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
    void modify_with(const ssmod_t &m);
  };

  /*! ship game object */
  class ship : public virtual physical_object, public ship_stats {
  public:
    typedef ship* ptr;
    static ptr create();
    static const std::string class_id;
    static std::list<std::string> all_classes();
    static std::string starting_ship;

    combid fleet_id; /*!< id of the ship's fleet */
    sfloat angle; /*!< ship's angle */
    sfloat load;
    ship_stats base_stats;
    sint passengers;
    sint is_landed;
    cost::res_t cargo;

    // game_object
    void pre_phase(game_data *g);
    void move(game_data *g);
    void post_phase(game_data *g);
    void on_remove(game_data *g);
    float vision();
    bool serialize(sf::Packet &p);
    bool is_active();
    ptr clone();
    bool isa(std::string c);
  
    // physical_object
    std::list<combid> confirm_interaction(std::string a, std::list<combid> t, game_data *g);
    std::set<std::string> compile_interactions();
    float interaction_radius();
    bool can_see(game_object::ptr x);

    // ship
    ship();
    ship(const ship &s);
    ship(const ship_stats &s);
    ~ship();

    void set_stats(ship_stats s);
    void receive_damage(game_object::ptr from, float damage);
    void on_liftoff(solar *from, game_data *g);
    bool has_fleet();
    bool accuracy_check(float a, ship::ptr t);

  protected:
    // serialised variables
    virtual game_object::ptr clone_impl();
    void copy_from(const ship &s);
  };
};
#endif
