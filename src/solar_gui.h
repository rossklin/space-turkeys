#ifndef _STK_SOLARGUI
#define _STK_SOLARGUI

#include <string>
#include <vector>
#include <functional>

#include "graphics.h"
#include "types.h"
#include "solar.h"
#include "research.h"

namespace st3{
  namespace solar{
    /*! names of available choice templates */
    typedef std::function<choice_t(solar)> template_func;
    static hm_t<std::string, template_func> choice_template;

    /*! blocking gui to generate a solar choice */
    bool gui(solar s, window_t &w, research &r, choice_t &c);
  };
};
#endif
