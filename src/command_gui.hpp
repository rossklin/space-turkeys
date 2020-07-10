#include <SFGUI/Widgets.hpp>
#include <SFML/Graphics.hpp>
#include <set>
#include <string>

#include "selector.hpp"
#include "types.hpp"

namespace st3 {
namespace client {
class game;
};

namespace interface {
/*! gui for allocating ships to command selectors */
class command_gui : public sfg::Window {
  struct class_info {
    int available;
    int allocated;
    sfg::Adjustment::Ptr adjust;
    sfg::Label::Ptr alloc_label;
  };

  hm_t<std::string, class_info> data;
  int selected_policy;

 public:
  typedef std::shared_ptr<command_gui> Ptr;
  typedef std::shared_ptr<const command_gui> PtrConst;
  static Ptr Create(client::command_selector::ptr c, client::game *g);
  static const std::string sfg_id;

  sfg::Box::Ptr layout;

  command_gui(client::command_selector::ptr c, client::game *g);
};
};  // namespace interface
};  // namespace st3
