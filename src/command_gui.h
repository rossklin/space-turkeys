#include <string>
#include <set>

#include <SFML/Graphics.hpp>
#include <SFGUI/Widgets.hpp>

#include "graphics.h"
#include "types.h"
#include "ship.h"
#include "selector.h"

namespace st3{
  namespace client {
    class game;
  };

  namespace interface {
    /*! gui for allocating ships to command selectors */
    class command_gui : public sfg::Window {
      hm_t<combid, sfg::Button::Ptr> ship_buttons;
      hm_t<std::string, sfg::Table::Ptr> tab_available;
      hm_t<std::string, sfg::Table::Ptr> tab_allocated;

    public:
      typedef std::shared_ptr<command_gui> Ptr;
      typedef std::shared_ptr<const command_gui> PtrConst;
      static Ptr Create(client::command_selector::ptr c, client::game *g);
      static const std::string sfg_id;

      sfg::Box::Ptr layout;

      command_gui(client::command_selector::ptr c, client::game *g);
    };
  };
};
