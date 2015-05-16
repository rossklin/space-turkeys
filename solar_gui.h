#ifndef _STK_SOLARGUI
#define _STK_SOLARGUI

#include <string>
#include <vector>
#include <functional>

#include "types.h"
#include "solar.h"
#include "research.h"

#include <TGUI/TGUI.hpp>

namespace st3{
  class solar_gui : public tgui::Gui{
    static constexpr float slider_count = 100;
    static std::vector<std::string> template_name;

    typedef std::function<float(int, float)> incfunc;
    struct control_group;

    struct control{
      tgui::Label::Ptr label;
      tgui::Slider::Ptr slider;
      tgui::Label::Ptr feedback;

      control(tgui::Gui &g);
      void setup(solar_gui::control_group *c,
		 int id,
		 std::string conf, 
		 point pos, 
		 point dims, 
		 std::string name, 
		 float base_value,
		 float value, 
		 float cap, 
		 float deriv);
      void update(float deriv);
    };

    struct control_group{
      solar_gui *sg;
      int id;
      incfunc incs;
      float cap;
      tgui::Label::Ptr label;      
      std::vector<control*> controls;

      control_group(solar_gui *g, 
		    int id,
		    std::string conf,
		    std::string title,
		    point pos,
		    point label_dims,
		    std::vector<std::string> names,
		    std::vector<float> values,
		    std::vector<float> proportions,
		    float total,
		    incfunc incs,
		    float c);
      ~control_group();
      void load_controls(float tot, std::vector<float> p);
      void update_value(tgui::Callback const &c);
      float get_sum();
    };

    sf::RenderWindow &window;
    std::vector<control_group*> controls;

    tgui::Label::Ptr header_population;
    tgui::ComboBox::Ptr tsel;

    bool done;
    bool accept;
    solar::solar s;
    research r;
    float round_time;

  public:
    solar::choice_t c;

    solar_gui(sf::RenderWindow &w, solar::solar s, solar::choice_t c, research r, float rt);
    ~solar_gui();
    void load_controls(solar::choice_t c);
    void callback_done(bool a);
    void callback_template();
    bool run();
    solar::choice_t evaluate();
    float get_control_cap(int id);
    void update_popsum();
    void load_template(std::string s);
    float compute_workers(float p);
    float compute_workers_nostarve(float p);
  };
};
#endif
