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
      sfg::Box::Ptr sub_layout;
      sfg::Box::Ptr info_layout;
      sfg::Label::Ptr tooltip;
      hm_t<std::string, sfg::Image::Ptr> sector_buttons;
      hm_t<std::string, sfg::Image::Ptr> military_buttons;
      point sub_dims;
      
      solar::ptr sol;

      solar_gui(solar::ptr s);
      void build_info();
      void setup(std::string v);
      sfg::Widget::Ptr setup_development();
      sfg::Widget::Ptr setup_sectors();
      sfg::Widget::Ptr setup_military();
    };
  };
};
  
#endif
