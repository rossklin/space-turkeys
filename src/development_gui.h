#ifndef _STK_DEVELOPMENT_GUI
#define _STK_DEVELOPMENT_GUI

#include <string>
#include <list>
#include <functional>
#include <SFGUI/SFGUI.hpp>
#include <SFGUI/Widgets.hpp>

#include "types.h"
#include "development_tree.h"

namespace st3 {
  namespace interface {
    class development_gui : public sfg::Bin {
    public:
      typedef std::shared_ptr<development_gui> Ptr;
      typedef std::shared_ptr<const development_gui> PtrConst;
      typedef std::function<std::list<std::string>(std::string)> f_req_t;
      typedef std::function<void(std::string)> f_select_t;

      static const std::string sfg_id;

      static Ptr Create(hm_t<std::string,
			development::node> map,
			std::string selected,
			f_req_t callback,
			f_select_t on_select,
			bool is_facility,
			int width);

      const std::string& GetName() const;
      sf::Vector2f CalculateRequisition();

    protected:
      std::set<std::string> dependent;
      std::set<std::string> available;
      hm_t<std::string, sfg::Button::Ptr> button_map;

      hm_t<std::string, development::node> map;
      std::string selected;
      f_req_t f_req;
      f_select_t on_select;
      int width;
      
      sfg::Box::Ptr layout;
      sfg::Frame::Ptr frame;
      
      development_gui(hm_t<std::string,
		      development::node> map,
		      std::string selected,
		      f_req_t callback,
		      f_select_t on_select,
		      bool is_facility,
		      int width);
      void setup();
      void reset_image(std::string v);
    };
  };
};

#endif
