#ifndef _STK_GAME_OBJECT
#define _STK_GAME_OBJECT

#include <set>
#include <list>
#include <memory>
#include <SFML/Network.hpp>

#include "types.h"
#include "command.h"

namespace st3{
  class game_data;
  
  class game_object{
  public:
    typedef std::shared_ptr<game_object> ptr;
    static const sint neutral_owner = -1;
    
    point position;
    sfloat radius;
    idtype owner;
    bool remove;
    combid id;

    game_object();
    virtual ~game_object();    
    virtual void pre_phase(game_data *g) = 0;
    virtual void move(game_data *g) = 0;
    virtual void interact(game_data *g) = 0;
    virtual void post_phase(game_data *g) = 0;
    virtual void on_add(game_data *g);
    virtual void on_remove(game_data *g);
    virtual float vision();
    virtual void serialize(sf::Packet &p);
    virtual void deserialize(sf::Packet &p);
    ptr clone();

    void update_position(game_data *g);

  protected:
    virtual ptr clone_impl() = 0;
  };

  class commandable_object : public virtual game_object{
  public:
    typedef std::shared_ptr<commandable_object> ptr;
    commandable_object();
    ~commandable_object();
    virtual void give_commands(std::list<command> c, game_data *g) = 0;
    ptr clone();
    
  protected:
    virtual game_object::ptr clone_impl() = 0;
  };
};

#endif
