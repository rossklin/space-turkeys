#ifndef _STK_SOLAR
#define _STK_SOLAR

#include <set>
#include <vector>
#include <list>
#include <rapidjson/document.h>

#include "types.h"
#include "ship.h"
#include "game_object.h"
#include "choice.h"
#include "development_tree.h"

namespace st3{
  class game_data;

  namespace research {
    struct data;
  };

  struct turret_t {
    sfloat range; /*!< radius in which the turret can fire */
    sfloat damage; /*!< turret's damage */
    sfloat accuracy;
    sfloat load;

    turret_t();
  };

  class facility : public virtual development::node{
  public:
    sfloat vision;
    sint is_turret;
    turret_t turret;
    sfloat base_hp;
    sfloat shield;
    sfloat water_usage;
    sfloat space_usage;
    cost::res_t cost_resources;

    facility();
    void read_from_json(const rapidjson::Value &v);
  };

  class facility_object : public facility {
  public:
    sfloat hp;
    sint level;

    facility_object();
    facility_object(const facility &f);
  };

  /*! data representing a solar system */
  class solar : public virtual physical_object, public virtual commandable_object{
  public:
    typedef solar* ptr;
    static ptr create();
    static const std::string class_id;
    static const hm_t<std::string, facility>& facility_table();
    
    choice::c_solar choice_data;
    float dt;

    hm_t<std::string, facility_object> development;

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
    cost::ship_allocation fleet_growth;
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
    std::list<std::string> available_facilities(const research::data &r);
    void develop(std::string fac);
    int get_facility_level(std::string fac);

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
    static constexpr float f_buildrate = 1e-2;
    static constexpr float f_devrate = 1e-2;
    static constexpr float f_resrate = 1e-2;

    void pay_resources(cost::res_t r);
    float resource_constraint(cost::res_t r);
    virtual game_object::ptr clone_impl();
    void copy_from(const solar &s);
  };
};
#endif
