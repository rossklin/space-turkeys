#ifndef _STK_RESEARCH_GUI
#define _STK_RESEARCH_GUI

#include <string>
#include <functional>
#include <SFGUI/SFGUI.hpp>
#include <SFGUI/Widgets.hpp>

#include "choice.h"
#include "research.h"
#include "choice_gui.h"

namespace st3 {
  namespace interface {
    sfg::Widget::Ptr research_gui();
  };
};
  
#endif
