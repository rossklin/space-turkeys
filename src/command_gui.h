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
      struct table_t {
	std::set<sfg::Button::Ptr> buttons;
	sfg::Box::Ptr layout;
	std::vector<sfg::Box::Ptr> rows;
	int cols;
	float padding;

	void setup(int ncols, float padding);
	void update_rows();
	void add_button(sfg::Button::Ptr b);
	void remove_button(sfg::Button::Ptr b);
      };
      
      hm_t<combid, sfg::Button::Ptr> ship_buttons;
      hm_t<std::string, table_t> tab_available;
      hm_t<std::string, table_t> tab_allocated;

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
