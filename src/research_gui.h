#ifndef _STK_RESEARCH_GUI
#define _STK_RESEARCH_GUI

#include <string>
#include <functional>
#include <SFGUI/SFGUI.hpp>
#include <SFGUI/Widgets.hpp>

#include "choice.h"
#include "research.h"

namespace st3 {
  namespace interface {

    // research choice window
    class research_gui : public sfg::Window {
    public:
      typedef std::shared_ptr<research_gui> Ptr;
      typedef std::shared_ptr<const research_gui> PtrConst;

      static const std::string sfg_id;
      static Ptr Create();

      sfg::Box::Ptr layout;

    protected:
      std::string response;
      research_gui();
    };
  };
};
  
#endif
