#ifndef _STK_CHOICE
#define _STK_CHOICE

#include <list>
#include <vector>


namespace st3{
  struct upgrade_choice{

  };

  struct command{

  };

  struct upgrade{

  };

  struct choice{
    std::list<upgrade_choice> upgrades;
    std::list<command> fleet_commands;
    std::list<command> solar_commands;

    void delete_selected();
    void increment_selected(int i);
    void lock_selected();
    void unlock_selected();
    std::list<command*> selected_solar_commands();
    std::list<command*> selected_fleet_commands();
    void clear_selection();
    upgrade total_upgrade();
  };
};
#endif
