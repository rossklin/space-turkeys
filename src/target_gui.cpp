#include "target_gui.hpp"

#include <functional>
#include <list>
#include <string>

#include "rsg/src/button.hpp"
#include "rsg/src/component.hpp"
#include "rsg/src/panel.hpp"
#include "rsg/src/utility.hpp"
#include "style.hpp"
#include "utility.hpp"

using namespace std;
using namespace RSG;
using namespace st3;

PanelPtr target_gui(sf::Vector2f position, std::list<std::string> options, std::function<void(std::string)> callback) {
  list<ComponentPtr> children = utility::map<list<string>, list<ComponentPtr>>(
      [callback](string v) -> ComponentPtr {
        return Button::create(v, [callback, v](ButtonPtr self) { callback(v); });
      },
      options);

  return styled<Panel>(
      {{"position", "fixed"}, {"top", to_string(position.y)}, {"left", to_string(position.x)}},
      children,
      Panel::ORIENT_VERTICAL);
}
