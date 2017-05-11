#ifndef _STK_SOLAR
#define _STK_SOLAR

#include <set>
#include <vector>
#include <list>

#include "types.h"
#include "ship.h"
#include "game_object.h"
#include "choice.h"

namespace st3{
  class game_data;

  struct turret_t {
    sfloat range; /*!< radius in which the turret can fire */
    sfloat damage; /*!< turret's damage */
    sfloat accuracy;
    sfloat load;

    turret_t();
  };

  class facility {
  public:
    std::string name;
    hm_t<std::string, sfloat> sector_boost;
    sfloat vision;
    sint is_turret;
    turret_t turret;
    sfloat hp;
    sfloat shield;
    hm_t<std::string, std::set<std::string> > ship_upgrades;

    // requirements
    cost::facility_cost cost;
    hm_t<std::string, int> depends_facilities;
    std::set<std::string> depends_techs;

    facility();
  };

  class facility_object : public facility {
  public:
    facility base_info;
    int level;

    facility_object();
  };

  class development_tree {
  public:
    static const hm_t<std::string, facility>& table();
    hm_t<std::string, facility_object> facilities;

    std::list<std::string> available();
  };

  /*! data representing a solar system */
  class solar : public virtual physical_object, public virtual commandable_object{
  public:
    typedef solar* ptr;
    static ptr create();
    static const std::string class_id;
    
    choice::c_solar choice_data;
    float dt;

    development_tree development;

    // points to spend on development
    sfloat research_points;
    sfloat development_points;
    
    sfloat water;
    sfloat space;
    sfloat ecology;
    sfloat population;
    sfloat happiness;

    cost::res_t available_resource; 
    cost::res_t resource_storage;
    cost::ship_allocation<sfloat> fleet_growth;
    std::set<combid> ships;

    solar();
    ~solar();

    // game_object
    void pre_phase(game_data *g);
    void move(game_data *g);
    void post_phase(game_data *g);
    bool serialize(sf::Packet &p);
    ptr clone();
    bool isa(std::string c);

    // physical_object
    std::list<combid> confirm_interaction(std::string a, std::list<combid> t, game_data *g);
    std::set<std::string> compile_interactions();
    float interaction_radius();

    // commandable_object
    void give_commands(std::list<command> c, game_data *g);

    // solar
    void receive_damage(game_object::ptr s, float damage, game_data *g);
    void damage_facilities(float d);
    float space_status();
    float water_status();
    float vision();
    bool has_defense();
    std::string get_info();
    float compute_boost(std::string sector);
    float compute_shield_power();

    // solar: increment functions
    float population_increment();
    float ecology_increment();
    float happiness_increment(choice::c_solar &c);
    float research_increment(choice::c_solar &c);
    float resource_increment(std::string v, choice::c_solar &c);
    float development_increment(choice::c_solar &c);
    float ship_increment(std::string v, choice::c_solar &c);
    float compute_workers();
    void dynamics(); 

  protected:
    static constexpr float f_growth = 4e-2;
    static constexpr float f_crowding = 2e-2;
    static constexpr float f_minerate = 1e-2;
    static constexpr float f_buildrate = 1e-1;

    void pay_resources(cost::res_t r);
    float resource_constraint(cost::res_t r);
    virtual game_object::ptr clone_impl();
    void copy_from(const solar &s);
  };
};
#endif
