#ifndef _STK_FLEET
#define _STK_FLEET

#include <set>
#include "types.h"
#include "command.h"
#include "game_object.h"
#include "interaction.h"
#include "ship_stats.h"

namespace st3{
  class game_data;

  namespace fleet_action{
    extern const std::string go_to;
    extern const std::string idle;
  };

  /*! a fleet controls a set of ships */
  class fleet : public virtual commandable_object{
  public:
    typedef fleet* ptr;
    static ptr create(idtype pid);
    static const std::string class_id;

    class suggestion {
    public:
      // fleet to ship suggestions
      static const sint summon = 1;
      static const sint engage = 2;
      static const sint scatter = 4;
      static const sint travel = 8;
      static const sint activate = 16;
      static const sint hold = 32;
      static const sint evade = 64;

      sint id;
      point p;

      suggestion();
      suggestion(sint i);
      suggestion(sint i, point p);
    };

    struct analytics {
      std::list<std::pair<point, float> > enemies;
      ssfloat_t average_ship;
      sbool converge;
      sfloat vision_buf;
      float speed_limit;
      point target_position;
      float spread_radius;
      float spread_density;
      point path;
      sbool can_evade;

      analytics();
    };

    static const idtype server_pid = -1;
    static const int update_period = 10; /*!< number of increments between fleet data updates */
    static const int interact_d2 = 100; /*!< distance from target at which the fleet converges */
    static constexpr float min_radius = 10; /*!< smallest allowed fleet radius (for visibility) */

    // fleet policies
    static const sint policy_aggressive;
    static const sint policy_reasonable;
    static const sint policy_evasive;
    static const sint policy_maintain_course;

    // serialized components
    std::set<combid> ships; /*!< ids of ships in fleet */
    std::set<std::string> interactions; /*!< set of available interactions */
    command com; /*!< the fleet's command (currently this only holds the target) */

    // mechanical components
    int update_counter; /*!< counter for updating fleet data */
    analytics stats;

    // game_object stuff
    void pre_phase(game_data *g);
    void move(game_data *g);
    void post_phase(game_data *g);
    float vision();
    bool serialize(sf::Packet &p);
    bool isa(std::string c);
    void on_remove(game_data *g);

    // commandable object stuff
    void give_commands(std::list<command> c, game_data *g);

    // fleet stuff
    fleet();
    fleet(idtype pid);
    ~fleet();
    bool is_idle();
    void set_idle();
    void analyze_enemies(game_data *g);
    void update_data(game_data *g, bool force_refresh = false);
    void remove_ship(combid i);
    suggestion suggest(combid i, game_data *g);
    float get_hp();
    float get_dps();
    float get_strength();
    
    ptr clone();

  protected:
    void check_waypoint(game_data *g);
    void check_in_sight(game_data *g);
    virtual game_object::ptr clone_impl();
    void copy_from(const fleet &s);
  };
};
#endif
