#ifndef _STK_CHOICE_GUI
#define _STK_CHOICE_GUI

#include <string>
#include <list>
#include <functional>
#include <SFGUI/SFGUI.hpp>
#include <SFGUI/Widgets.hpp>

#include "types.h"

namespace st3 {
  namespace interface {
    struct choice_info {
      bool available;
      float progress;
      std::list<std::string> info;
      std::list<std::string> requirements;

      choice_info();
    };
    
    class choice_gui : public sfg::Window {
    public:
      typedef std::shared_ptr<choice_gui> Ptr;
      typedef std::shared_ptr<const choice_gui> PtrConst;
      typedef std::function<choice_info(std::string)> f_info_t;
      typedef std::function<void(hm_t<std::string, bool>, bool)> f_result_t;

      static const std::string sfg_id;
      static const std::string class_normal;
      static const std::string class_selected;

      static Ptr Create(std::string title,
			std::string help_text,
			bool unique,
			hm_t<std::string, bool> options,
			f_info_t info,
			f_result_t callback);
      
      static hm_t<std::string, int> calc_faclev();

      const std::string& GetName() const;

    protected:
      bool unique;
      hm_t<std::string, bool> options;
      hm_t<std::string, sfg::Button::Ptr> buttons;
      sfg::Box::Ptr info_area;
      f_info_t f_info;
      f_result_t callback;
      
      int width;
      sfg::Box::Ptr layout;
      sfg::Frame::Ptr frame;

      choice_gui(std::string title,
		 std::string help_text,
		 bool unique,
		 hm_t<std::string, bool> options,
		 f_info_t info,
		 f_result_t callback);

      void update_selected();
    };

    sfg::Widget::Ptr governor_gui(std::list<solar::ptr> solars);
    sfg::Widget::Ptr military_gui();
    sfg::Widget::Ptr research_gui();
  };
};

#endif