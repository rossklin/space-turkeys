#pragma once

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

  /*! data representing a solar system */
  class solar : public virtual physical_object, public virtual commandable_object{
  public:
    typedef solar* ptr;
    static ptr create(idtype id, point p, float bounty, float var = 0.3);
    static const std::string class_id;
    
    choice::c_solar choice_data;
    research::data *research_level;

    hm_t<std::string, int> development;
    sfloat ship_progress;
    sfloat build_progress;
    sfloat research_points;
    sfloat hp;
    cost::res_t resources;
    
    sbool was_discovered;
    std::set<idtype> known_by;

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
    float vision();
    std::string get_info();
    float effective_level(std::string k);
    void dynamics(game_data *g);
    float max_hp();
    cost::res_t devcost(std::string k);
    float devtime(std::string k);
    float population();

  protected:
    static const float f_growth;
    static const float f_crowding;
    static const float f_minerate;
    static const float f_buildrate;
    static const float f_devrate;
    static const float f_resrate;

    void pay_resources(cost::res_t r);
    bool can_afford(cost::res_t r);
  };
};
