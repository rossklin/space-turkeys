#ifndef _STK_FLEET
#define _STK_FLEET

#include <set>
#include "types.h"
#include "command.h"
#include "game_object.h"
#include "interaction.h"

namespace st3{
  class game_data;

  namespace fleet_action{
    extern const std::string land;
    extern const std::string turret_combat;
    extern const std::string space_combat;
    extern const std::string bombard;
    extern const std::string colonize;
    extern const std::string go_to;
    extern const std::string join;
    extern const std::string idle;
  };

  /*! a fleet controls a set of ships */
  class fleet : public virtual commandable_object{
  public:
    typedef fleet* ptr;
    static ptr create();
    static const std::string class_id;

    static hm_t<std::string, target_condition> &action_condition_table();
    static std::set<std::string> all_interactions();
    static std::set<std::string> all_base_actions();
    
    static const int update_period = 1; /*!< number of increments between fleet data updates */
    static const int interact_d2 = 100; /*!< distance from target at which the fleet converges */
    static constexpr float min_radius = 10; /*!< smallest allowed fleet radius (for visibility) */

    // serialized components
    std::set<combid> ships; /*!< ids of ships in fleet */
    std::set<std::string> interactions; /*!< set of available interactions */
    command com; /*!< the fleet's command (currently this only holds the target) */
    sbool converge; /*!< whether the fleet is converging on it's target */
    sfloat vision_buf;

    // mechanical components
    int update_counter; /*!< counter for updating fleet data */
    float speed_limit; /*!< speed of slowest ship in fleet */
    point target_position;
    
    /*! default constructor */
    fleet();
    ~fleet();

    // game_object stuff
    void pre_phase(game_data *g);
    void move(game_data *g);
    bool confirm_interaction(std::string a, combid t, game_data *g);
    std::set<std::string> compile_interactions();
    void post_phase(game_data *g);
    float vision();
    bool serialize(sf::Packet &p);

    // commandable object stuff
    void give_commands(std::list<command> c, game_data *g);

    // fleet stuff
    bool is_idle();
    void set_idle();
    void update_data(game_data *g);
    void remove_ship(combid i);
    bool confirm_ship_interaction(std::string a, combid t);
    float interaction_radius();
    
    ptr clone();

  protected:
    void check_waypoint(game_data *g);
    void check_in_sight(game_data *g);
    virtual game_object::ptr clone_impl();
    void copy_from(const fleet &s);
  };
};
#endif
