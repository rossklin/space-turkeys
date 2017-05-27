#ifndef _STK_DESKTOP
#define _STK_DESKTOP

#include <SFGUI/SFGUI.hpp>
#include <SFGUI/Widgets.hpp>

#include "choice.h"
#include "research.h"

namespace st3 {
  namespace client{
    class game;
  };
  
  namespace interface {

    // main interface
    class main_interface : public sfg::Desktop {	
    public:
      // desktop geometry data
      static sf::Vector2u desktop_dims;
      static sf::FloatRect qw_allocation;
      static int top_height;
      static int bottom_start;
      std::list<std::function<void(void)> > post_process;

      sfg::Widget::Ptr query_window;
      sfg::Label::Ptr hover_label;

      client::game *g;

      // data for generating the client's choice
      choice::choice response;

      // progress communication variables for game loop
      bool accept;
      bool done;

      main_interface(sf::Vector2u dims, client::game *g);
      void HandleEvent(const sf::Event& event);
      void reset_qw(sfg::Widget::Ptr p);
      void clear_qw();
      research::data get_research();
      void bind_ppc(sfg::Widget::Ptr w, std::function<void(void)> f);
    };

    extern main_interface *desktop;
  };
};

#endif
