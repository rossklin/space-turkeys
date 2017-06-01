#ifndef _STK_SOLAR_GUI
#define _STK_SOLAR_GUI

#include <string>
#include <functional>
#include <SFGUI/SFGUI.hpp>
#include <SFGUI/Widgets.hpp>

#include "choice.h"
#include "solar.h"

namespace st3 {
  namespace interface {

    // solar choice window
    class solar_gui : public sfg::Window {
    public:
      typedef std::shared_ptr<solar_gui> Ptr;
      typedef std::shared_ptr<const solar_gui> PtrConst;

      static const std::string sfg_id;
      static const std::string tab_sectors;
      static const std::string tab_development;
      static const std::string tab_military;
      choice::c_solar response;

      static Ptr Create(solar::ptr s);

    protected:
      // sub interface tracker
      sfg::Box::Ptr layout;
      sfg::Box::Ptr choice_layout;
      sfg::Box::Ptr sub_layout;
      sfg::Box::Ptr info_layout;
      sfg::Label::Ptr tooltip;

      solar::ptr sol;

      solar_gui(solar::ptr s);
      void build_info();
      void build_choice();
      sfg::Box::Ptr new_sub(std::string v);
      void build_military();
      void build_mining();

      sfg::Button::Ptr priority_button(std::string label, float &data, std::function<bool()> inc_val, sfg::Label::Ptr tip = 0);
    };
  };
};
  
#endif
