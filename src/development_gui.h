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
    class development_gui : public sfg::Window {
    public:
      typedef std::shared_ptr<development_gui> Ptr;
      typedef std::shared_ptr<const development_gui> PtrConst;
      typedef std::function<std::list<std::string>(std::string)> f_req_t;
      typedef std::function<void(bool, std::string)> f_complete_t;

      static const std::string sfg_id;

      static Ptr Create(hm_t<std::string, development::node> map, f_req_t callback, f_complete_t on_complete, bool is_facility);

    protected:
      sfg::Frame::Ptr frame;
      development_gui(hm_t<std::string, development::node> map, f_req_t callback, f_complete_t on_complete, bool is_facility);
    };
  };
};

#endif
