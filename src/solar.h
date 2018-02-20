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
    sfloat load_time;

    turret_t();
    float accuracy_check(ship::ptr t, float d);
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
    sfloat water_provided;
    sfloat space_provided;
    cost::res_t cost_resources;

    facility();
    facility(const facility &f);
    void read_from_json(const rapidjson::Value &v);
  };

  class facility_object : public facility {
  public:
    sfloat hp;

    facility_object();
    facility_object(const facility &f);
  };

  /*! data representing a solar system */
  class solar : public virtual physical_object, public virtual commandable_object{
  public:
    typedef solar* ptr;
    static ptr create(idtype id, point p, float bounty, float var = 0.3);
    static const std::string class_id;
    static const hm_t<std::string, facility>& facility_table();
    
    choice::c_solar choice_data;
    float dt;
    float threat_level;
    research::data *research_level;

    hm_t<std::string, facility_object> development;

    // points to spend on development
    sfloat research_points;
    
    sfloat water;
    sfloat space;
    sfloat ecology;
    sfloat population;
    sfloat happiness;
    sbool out_of_resources;
    sbool was_discovered;
    std::set<idtype> known_by;

    cost::res_t available_resource; 
    cost::res_t resource_storage;
    float ship_progress;
    std::set<combid> ships;

    solar() = default;
    ~solar() = default;
    solar(const solar &s);

    // game_object
    void pre_phase(game_data *g);
    void move(game_data *g);
    void post_phase(game_data *g);
    bool serialize(sf::Packet &p);
    game_object::ptr clone();
    bool isa(std::string c);

    // physical_object
    std::set<std::string> compile_interactions();
    float interaction_radius();
    bool can_see(game_object::ptr x);
    
    // commandable_object
    void give_commands(std::list<command> c, game_data *g);

    // solar
    void receive_damage(game_object::ptr s, float damage, game_data *g);
    void damage_facilities(float d);
    float space_status();
    float water_status();
    float crowding_rate();
    float base_growth();
    float vision();
    bool has_defense();
    std::string get_info();
    float compute_boost(std::string sector);
    float compute_shield_power();
    float compute_hp_ratio();
    std::list<std::string> list_facility_requirements(std::string v, const research::data &r);
    std::list<std::string> available_facilities(const research::data &r);
    bool develop(std::string fac);
    std::list<facility_object> developed();
    facility_object developed(std::string v, int lev_inc = 0);
    std::list<facility_object*> facility_access();
    facility_object *facility_access(std::string key);
    
    // solar: increment functions
    float population_increment();
    float ecology_increment();
    float happiness_increment(choice::c_solar &c);
    float research_increment(choice::c_solar &c);
    float resource_increment(std::string v, choice::c_solar &c);
    float development_increment(choice::c_solar &c);
    float ship_increment(choice::c_solar &c);
    float compute_workers();
    choice::c_solar government();
    void dynamics(); 

  protected:
    static const float f_growth;
    static const float f_crowding;
    static const float f_minerate;
    static const float f_buildrate;
    static const float f_devrate;
    static const float f_resrate;

    void pay_resources(cost::res_t r);
    float resource_constraint(cost::res_t r);
  };
};
#endif
