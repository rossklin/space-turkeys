#ifndef _STK_SOLARGUI
#define _STK_SOLARGUI

#include <string>
#include <vector>

#include "types.h"
#include "solar.h"

#include <TGUI/TGUI.hpp>

namespace st3{
  class solar_gui : public tgui::Gui{
    sf::RenderWindow &window;
    tgui::Tab::Ptr tabs;
    hm_t<std::string, tgui::Panel::Ptr> panels;
    std::vector<std::string> topics;
    bool done;
    bool accept;

  public:

    solar_gui(sf::RenderWindow &w, solar::solar &s);
    void callback_accept();
    void callback_cancel();
    void callback_panel(const tgui::Callback &c);
    bool run(solar::choice_t &c);
  };
};
#endif
