#ifndef _STK_WAYPOINT
#define _STK_WAYPOINT

#include <list>

#include "types.h"
#include "command.h"
#include "game_object.h"

namespace st3{
  class game_data;
  
  /*! Waypoints allow position based fleet joining and splitting.*/
  class waypoint : public virtual game_object {
  public:
    typedef std::shared_ptr<waypoint> ptr;
    static ptr create(idtype owner);
    
    /*! List of commands waiting to trigger when all ships have arrived */
    std::list<command> pending_commands;
    std::set<combid> ships;

    waypoint();
    waypoint(idtype o);
    ~waypoint();
    void pre_phase(game_data *g);
    void move(game_data *g);
    void interact(game_data *g);
    void post_phase(game_data *g);
    float vision();

    ptr clone();

  protected:
    virtual game_object::ptr clone_impl();
  };
};
#endif
