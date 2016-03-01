#ifndef _STK_GAME_OBJECT
#define _STK_GAME_OBJECT

#include <memory>
#include "types.h"

namespace st3{
  class game_data;
  
  class game_object{
    static sint id_counter = 0;
  public:
    typedef std::shared_ptr<game_object> ptr;
    
    point position;
    sfloat radius;
    idtype owner;
    bool remove;
    identifier::combid id;

    game_object();
    virtual ~game_object();    
    virtual void pre_phase(game_data *g) = 0;
    virtual void move(game_data *g) = 0;
    virtual void interact(game_data *g) = 0;
    virtual void post_phase(game_data *g) = 0;
    virtual void on_add(gamd_data *g);
    virtual void on_remove(gamd_data *g);
    virtual float vision();

    void update_position(game_data *g);
    virtual std::set<interaction> compile_interactions();
  };

  class commandable_object : public virtual game_object{
    commandable_object();
    ~commandable_object();
    virtual void give_commands(std::list<command> c, game_data *g) = 0;
  };
};

#endif
