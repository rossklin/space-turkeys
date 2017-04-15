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
    const std::string space_combat;
    const std::string bombard;
    const std::string colonize;
    const std::string go_to;
    const std::string join;
    const std::string follow;
    const std::string idle;
  };

  /*! a fleet controls a set of ships */
  class fleet : public virtual commandable_object{
  public:
    typedef std::shared_ptr<fleet> ptr;
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
    void interact(game_data *g);
    void post_phase(game_data *g);
    float vision();
    bool serialize(sf::Packet &p);

    // commandable object stuff
    void give_commands(std::list<command> c, game_data *g);

    // fleet stuff
    bool is_idle();
    void update_data(game_data *g);
    target_condition current_target_condition(game_data *g);
    void update_interactions(game_data *g);
    
    ptr clone();

  protected:
    void check_waypoint(game_data *g);
    void check_join(game_data *g);
    void check_in_sight(game_data *g);
    virtual game_object::ptr clone_impl();
    void copy_from(const fleet &s);
  };
};
#endif
