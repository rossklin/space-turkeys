#ifndef _STK_FLEET
#define _STK_FLEET

#include <set>
#include "types.h"
#include "command.h"
#include "game_object.h"
#include "interaction.h"

namespace st3{
  class game_data;

  /*! a fleet controls a set of ships */
  class fleet : public virtual commandable_object{
  public:
    typedef std::shared_ptr<fleet> ptr;
    static ptr create();
    static const std::string class_id;

    static target_condition action_condition_table(std::string a, idtype o);

    class action{
    public:
      static const std::string space_combat;
      static const std::string bombard;
    };
    
    static const int update_period = 1; /*!< number of increments between fleet data updates */
    static const int interact_d2 = 100; /*!< distance from target at which the fleet converges */
    static constexpr float min_radius = 10; /*!< smallest allowed fleet radius (for visibility) */

    // serialized components
    std::set<combid> ships; /*!< ids of ships in fleet */
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
    void pre_phase(game_data *g);
    void move(game_data *g);
    void interact(game_data *g);
    void post_phase(game_data *g);

    float vision();
    void give_commands(std::list<command> c, game_data *g);
    bool is_idle();
    void update_data(game_data *g);
    
    ptr clone();

  protected:
    void check_waypoint(game_data *g);
    void check_join(game_data *g);
    void check_in_sight(game_data *g);
    virtual game_object::ptr clone_impl();
  };
};
#endif
