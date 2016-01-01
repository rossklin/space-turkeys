#ifndef _STK_SOLARGUI
#define _STK_SOLARGUI

#include <string>
#include <vector>
#include <functional>

#include <SFGUI/SFGUI.hpp>
#include <SFGUI/Widgets.hpp>
#include <SFML/Graphics.hpp>

#include "graphics.h"
#include "types.h"
#include "solar.h"
#include "research.h"

namespace st3{
  namespace solar{
    namespace gui{

      template<typename C>
      class base : public sfg::Window{
	research::data research_level;
      public:
	
	C response;
	bool accept;
	bool done;
      };
      
      class s_military : public base<choice::c_military>{
      public:      
	typedef std::shared_ptr<base> Ptr;
	typedef std::shared_ptr<const base> PtrConst;

	static Ptr Create(choice::c_military c, research::data r) override;
	
      protected:
	s_military(choice::c_military c, research::data r);
      };

      class s_mining : public base<choice::c_mining>{
      public:      
	typedef std::shared_ptr<base> Ptr;
	typedef std::shared_ptr<const base> PtrConst;

	static Ptr Create(choice::c_mining c, research::data r) override;
	
      protected:
	s_mining(choice::c_mining c, research::data r);
      };
      
      class main_window : base<choice::choice_t>{
      public:      
	typedef std::shared_ptr<base> Ptr;
	typedef std::shared_ptr<const base> PtrConst;

	static Ptr Create(choice::choice_t c, research::data r, solar s) override;
	
      protected:
	main_window(choice::choice_t c, research::data r, solar s);
      };
    };
  };
};
#endif
