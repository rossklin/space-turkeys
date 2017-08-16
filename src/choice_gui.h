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
    };
    
    class choice_gui : public sfg::Bin {
    public:
      typedef std::shared_ptr<choice_gui> Ptr;
      typedef std::shared_ptr<const choice_gui> PtrConst;
      typedef std::function<choice_info(std::string)> f_info_t;
      typedef std::function<void(std::set<std::string>, bool)> f_result_t;

      static const std::string sfg_id;
      static const std::string class_normal;
      static const std::string class_selected;

      static Ptr Create(std::set<std::string> options, f_info_t info, f_result_t callback);

      const std::string& GetName() const;
      sf::Vector2f CalculateRequisition();

    protected:
      bool unique;
      std::set<std::string> options;
      hm_t<std::string, sfg::Button::Ptr> buttons;
      sfg::Box::Ptr info_area;
      f_info_t f_info;
      f_result_t callback;
      std::set<std::string> selected;
      
      int width;
      sfg::Box::Ptr layout;
      sfg::Frame::Ptr frame;
      
      choice_gui(std::string title, bool unique, std::set<std::string> options, f_info_t info, f_result_t callback);
      void update_selected();
      void setup();
    };
  };
};

#endif
