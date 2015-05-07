#ifndef _STK_SOLARGUI
#define _STK_SOLARGUI

#include <string>
#include <vector>

#include "types.h"
#include "solar.h"

#include <TGUI/TGUI.hpp>

namespace st3{
  class solar_gui : public tgui::Gui{
    static constexpr float slider_count = 100;
    static float slider_sum(std::vector<tgui::Slider::Ptr> &v);

    sf::RenderWindow &window;
    tgui::Tab::Ptr tabs;
    hm_t<std::string, tgui::Panel::Ptr> panels;
    std::vector<std::string> topics;
    std::vector<tgui::Slider::Ptr> population_slider;
    std::vector<tgui::Slider::Ptr> industry_slider;
    std::vector<tgui::Slider::Ptr> research_slider;
    std::vector<tgui::Slider::Ptr> resource_slider;
    std::vector<tgui::Slider::Ptr> ship_slider;
    tgui::Label::Ptr header_population;
    tgui::Label::Ptr header_industry;
    tgui::Label::Ptr header_research;
    tgui::Label::Ptr header_resource;
    tgui::Label::Ptr header_ship;

    bool done;
    bool accept;
    solar::solar s;

  public:

    solar_gui(sf::RenderWindow &w, solar::solar &s);
    void callback_done(bool a);
    void callback_panel(const tgui::Callback &c);
    void update_info();
    bool run(solar::choice_t &c);
    void setup_controls(solar::choice_t c);
    solar::choice_t evaluate();
  };
};
#endif
