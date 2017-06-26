#ifndef _STK_WAYPOINT
#define _STK_WAYPOINT

#include <list>

#include "types.h"
#include "command.h"
#include "game_object.h"

namespace st3{
  class game_data;
  
  /*! Waypoints allow position based fleet joining and splitting.*/
  class waypoint : public virtual commandable_object {
  public:
    typedef waypoint* ptr;
    static ptr create(idtype owner);
    static const std::string class_id;
    
    /*! List of commands waiting to trigger when all ships have arrived */
    std::list<command> pending_commands;

    // game_object
    void pre_phase(game_data *g);
    void move(game_data *g);
    void post_phase(game_data *g);
    float vision();
    bool serialize(sf::Packet &p);
    game_object::ptr clone();
    bool isa(std::string c);

    // commandable_object
    void give_commands(std::list<command> c, game_data *g);

    // waypoint
    waypoint(idtype o);
    waypoint() = default;
    ~waypoint() = default;
    waypoint(const waypoint &w);
  };
};
#endif
