#ifndef _STK_GRAPHICS
#define _STK_GRAPHICS

#include <string>

#include <SFGUI/SFGUI.hpp>
#include <SFGUI/Widgets.hpp>
#include <SFML/Graphics.hpp>

#include "game_data.h"
#include "choice.h"
#include "types.h"
#include "ship.h"
#include "research.h"

namespace st3{
  /*! type representing a window */
  typedef sf::RenderWindow window_t;

  /*! graphics related code elements */
  namespace graphics{
    /*! a default font */
    extern sf::Font default_font;

    const sf::Color solar_neutral(150,150,150); /*!< color for neutral solars */
    const sf::Color command_selected_head(255,255,255,255);/*!< color for selected command head */
    const sf::Color command_selected_body(150,240,150,230);/*!< color for selected command body */
    const sf::Color command_selected_tail(80,200,80,130);/*!< color for selected command tail */
    const sf::Color command_selected_text(200,200,200);/*!< color for selected command text */
    const sf::Color command_normal_head(255,255,255,200);/*!< color for normal command head */
    const sf::Color command_normal_body(100,200,100,200);/*!< color for normal command body */
    const sf::Color command_normal_tail(50,100,50,100);/*!< color for normal command tail */
    const sf::Color command_normal_text(200,200,200,100);/*!< color for normal command text */
    const sf::Color solar_fill(10,20,30,40); /*!< color for solar interior */
    const sf::Color solar_selected(255,255,255,180); /*!< color for selected solar border */
    const sf::Color solar_selected_fill(200,225,255,80); /*!< color for selected solar interior */
    const sf::Color fleet_fill(200,200,200,50); /*!< color for fleet interior */
    const sf::Color fleet_outline(40,60,180,200); /*!< color for fleet border */

    /*! make an sf::Color from an sint 
      @param c 32 bit color representation
      @return sf::Color representation
    */
    sf::Color sfcolor(sint c);

    /*! compute a linear interpolation between two colors
      @param from first color
      @param to second color
      @param r proportion between from and to
      @return sf::Color "from + r * (to - from)"
    */
    sf::Color fade_color(sf::Color from, sf::Color to, float r);

    /*! initialize static graphics i.e. default font */
    void initialize();

    /*! draw a ship to a window 
      @param w window
      @param s ship
      @param c color
      @param sc scale
    */
    void draw_ship(window_t &w, ship s, sf::Color c, float sc = 1);

    namespace interface{

      // base class for choice windows
      template<typename C, typename R>
      class query : public C{
      public:
	R response;
      };

      // bottom panel
      class bottom_panel : public query<sfg::Window, choice::choice>{
	typedef std::shared_ptr<bottom_panel> Ptr;
	typedef std::shared_ptr<const bottom_panel> PtrConst;

	static Ptr Create() override;
	
      protected:
	bottom_panel();	
      };

      // top panel
      class top_panel : public sfg::Window{
	typedef std::shared_ptr<top_panel> Ptr;
	typedef std::shared_ptr<const top_panel> PtrConst;

	static Ptr Create() override;
	
      protected:
	top_panel();	
      };

      // research window
      class research_window : public query<sfg::Window, choice::c_research>{
      public:
	typedef std::shared_ptr<research_window> Ptr;
	typedef std::shared_ptr<const research_window> PtrConst;

	static Ptr Create(choice::c_research c) override;
	
      protected:
	research_window(choice::c_research c);
      };

      // solar choice windows
      namespace solar_query{
	// military choice sub window
	class military : public query<sfg::Box, solar::choice::c_military>{
	public:      
	  typedef std::shared_ptr<military> Ptr;
	  typedef std::shared_ptr<const military> PtrConst;

	  static Ptr Create(solar::choice::c_military c) override;
	
	protected:
	  military(solar::choice::c_military c);
	};

	// mining choice sub window
	class mining : public query<sfg::Box, solar::choice::c_mining>{
	public:      
	  typedef std::shared_ptr<mining> Ptr;
	  typedef std::shared_ptr<const mining> PtrConst;

	  static Ptr Create(solar::choice::c_mining c) override;
	
	protected:
	  mining(solar::choice::c_mining c);
	};

	// mining choice sub window
	class expansion : public query<sfg::Box, solar::choice::c_expansion>{
	public:      
	  typedef std::shared_ptr<expansion> Ptr;
	  typedef std::shared_ptr<const expansion> PtrConst;

	  static Ptr Create(solar::choice::c_expansion c) override;
	
	protected:
	  expansion(solar::choice::c_expansion c);
	};

	// main window
	class main_window : query<sfg::Window, solar::choice::choice_t>{
	  Box::Ptr layout;
	public:
	  typedef std::shared_ptr<query> Ptr;
	  typedef std::shared_ptr<const query> PtrConst;

	  int id;

	  static Ptr Create(int id, solar s) override;
	
	protected:
	  main_window(int id, solar s);
	};
      };

      // main interface
      class main_interface : public sfg::Desktop {
	sfg::Widget::Ptr query_window;

      public:
	research::data research_level;
	choice::choice response;
	bool accept;
	bool done;

	main_interface(choice::choice c, research::data r);
	void reset_query_window(sfg::Widget::Ptr p);
      };

      extern main_interface *desktop;
    };
  };
};
#endif
