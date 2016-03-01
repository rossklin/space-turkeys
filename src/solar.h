#ifndef _STK_SOLAR
#define _STK_SOLAR

#include <set>
#include <vector>
#include <list>

#include "types.h"
#include "ship.h"
#include "turret.h"
#include "game_object.h"

namespace st3{
  class game_data;

  namespace choice{
    struct c_solar;
  };

  /*! data representing a solar system */
  class solar : public virtual commandable_object{
  public:
    typedef std::shared_ptr<solar> ptr;
    static ptr create();
    
    choice::c_solar choice_data;
    float dt;
      
    cost::ship_allocation<sfloat> fleet_growth;
    cost::turret_allocation<sfloat> turret_growth;
    std::list<turret> turrets;
    hm_t<combid, ship::ptr> ships;
    sfloat research;
    sfloat water;
    sfloat space;
    sfloat ecology;
    cost::resource_allocation<cost::resource_data> resource;
    cost::sector_allocation<sfloat> sector;
    sfloat population;
    sfloat happiness;
    hm_t<idtype, float> damage_taken;
    hm_t<idtype, idtype> colonization_attempts;

    solar();
    ~solar();

    void pre_phase(game_data *g);
    void move(game_data *g);
    void interact(game_data *g);
    void post_phase(game_data *g);
    void give_commands(std::list<command> c, game_data *g);

    float space_status();
    float water_status();
    float vision();
    bool has_defense();
    void damage_turrets(float d);
    std::string get_info();

    // increment functions
    float poluation_increment();
    float ecology_increment();
    float happiness_increment(choice::c_solar &c);
    float research_increment(choice::c_solar &c);
    float resource_increment(std::string v, choice::c_solar &c);
    float expansion_increment(std::string v, choice::c_solar &c);
    float ship_increment(std::string v, choice::c_solar &c);
    float turret_increment(std::string v, choice::c_solar &c);

  protected:
    static const float f_growth = 4e-2;
    static const float f_crowding = 2e-2;
    static const float f_minerate = 1e-2;
    static const float f_buildrate = 1e-1;

    solar dynamics(); 
    void pay_resources(cost::resource_allocation<float> r);
    float resource_constraint(cost::resource_allocation<sfloat> r);
  };
};
#endif
