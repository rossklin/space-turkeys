#ifndef _STK_FLEET
#define _STK_FLEET

#include <set>
#include "types.h"
#include "command.h"
#include "game_object.h"

namespace st3{
  class game_data;

  /*! a fleet controls a set of ships */
  class fleet : public virtual commandable_object{
  public:
    typedef std::shared_ptr<fleet> ptr;
    static ptr create();
    
    static const int update_period = 1; /*!< number of increments between fleet data updates */
    static const int interact_d2 = 100; /*!< distance from target at which the fleet converges */
    static constexpr float min_radius = 10; /*!< smallest allowed fleet radius (for visibility) */

    // serialized components
    std::set<idtype> ships; /*!< ids of ships in fleet */
    command com; /*!< the fleet's command (currently this only holds the target) */
    sbool converge; /*!< whether the fleet is converging on it's target */

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

    /*! whether the fleet is idle i.e. at a waypoint or missing it's target */
    bool is_idle();
  };
};
#endif
