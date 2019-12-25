#ifndef _STK_FLEET
#define _STK_FLEET

#include <set>
#include "command.h"
#include "game_object.h"
#include "interaction.h"
#include "ship_stats.h"
#include "types.h"

namespace st3 {
class game_data;

namespace fleet_action {
extern const std::string go_to;
extern const std::string idle;
};  // namespace fleet_action

/*! a fleet controls a set of ships */
class fleet : public virtual commandable_object {
 public:
  typedef fleet *ptr;
  static ptr create(idtype pid, idtype idx);
  static const std::string class_id;

  /* class suggestion { */
  /* public: */
  /*   // fleet to ship suggestions */
  /*   static const sint summon; */
  /*   static const sint engage; */
  /*   static const sint scatter; */
  /*   static const sint travel; */
  /*   static const sint activate; */
  /*   static const sint hold; */
  /*   static const sint evade; */

  /*   sint id; */
  /*   point p; */

  /*   suggestion(); */
  /*   suggestion(sint i); */
  /*   suggestion(sint i, point p); */
  /* }; */

  struct analytics {
    std::list<std::pair<point, float> > enemies;
    ssfloat_t average_ship;
    sbool converge;
    sfloat vision_buf;
    float speed_limit;
    point target_position;
    float spread_radius;
    float spread_density;
    point evade_path;
    sbool can_evade;
    float facing_ratio;

    analytics();
  };

  static const idtype server_pid;
  static const int update_period; /*!< number of increments between fleet data updates */
  static const int interact_d2;   /*!< distance from target at which the fleet converges */

  // fleet policies
  static const sint policy_aggressive;
  static const sint policy_evasive;
  static const sint policy_maintain_course;

  static sint default_policy(std::string action);

  // serialized components
  std::set<combid> ships;             /*!< ids of ships in fleet */
  std::set<std::string> interactions; /*!< set of available interactions */
  command com;                        /*!< the fleet's command (currently this only holds the target) */
  point heading;
  bool pop_heading;
  std::list<point> path;
  /* suggestion suggest_buf; */

  // mechanical components
  int update_counter; /*!< counter for updating fleet data */
  analytics stats;
  bool force_refresh;

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
  fleet(idtype pid, idtype idx);
  fleet() = default;
  ~fleet() = default;
  fleet(const fleet &f);

  bool is_idle();
  void set_idle();
  void analyze_enemies(game_data *g);
  void update_data(game_data *g, bool set_force_refresh = false);
  void remove_ship(combid i);
  /* suggestion suggest(game_data *g); */
  float get_hp();
  float get_dps();
  float get_strength();
  void refresh_ships(game_data *g);
  void check_action(game_data *g);
  game_object::ptr clone();

 protected:
  void check_waypoint(game_data *g);
  void check_in_sight(game_data *g);
};
};  // namespace st3
#endif
