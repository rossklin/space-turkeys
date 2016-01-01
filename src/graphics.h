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

      extern research::data *research_level;
      extern Desktop *desktop;

      // base class for choice windows
      template<typename C>
      class query : public sfg::Window{
      public:
	C response;
	bool accept;
	bool done;
      };

      // main interface
      class main_interface : public query<choice::choice>{
	typedef std::shared_ptr<main_interface> Ptr;
	typedef std::shared_ptr<const main_interface> PtrConst;

	static Ptr Create(choice::choice c, research::data r) override;
	
      protected:
	main_interface(choice::choice c, research::data r);	
      };

      // research window
      class research_window : public query<choice::c_research>{
      public:
	typedef std::shared_ptr<research_window> Ptr;
	typedef std::shared_ptr<const research_window> PtrConst;

	static Ptr Create(choice::c_research c, data r) override;
	
      protected:
	research_window(choice::c_research c, data r);
      };

      // solar choice windows
      namespace solar_query{
	// military choice sub window
	class military : public query<solar::choice::c_military>{
	public:      
	  typedef std::shared_ptr<military> Ptr;
	  typedef std::shared_ptr<const military> PtrConst;

	  static Ptr Create(solar::choice::c_military c, research::data r) override;
	
	protected:
	  military(solar::choice::c_military c, research::data r);
	};

	// mining choice sub window
	class mining : public query<solar::choice::c_mining>{
	public:      
	  typedef std::shared_ptr<mining> Ptr;
	  typedef std::shared_ptr<const mining> PtrConst;

	  static Ptr Create(solar::choice::c_mining c, research::data r) override;
	
	protected:
	  mining(solar::choice::c_mining c, research::data r);
	};

	// mining choice sub window
	class expansion : public query<solar::choice::c_expansion>{
	public:      
	  typedef std::shared_ptr<expansion> Ptr;
	  typedef std::shared_ptr<const expansion> PtrConst;

	  static Ptr Create(solar::choice::c_expansion c, research::data r) override;
	
	protected:
	  expansion(solar::choice::c_expansion c, research::data r);
	};

	// main window
	class main_window : query<solar::choice::choice_t>{
	public:
	  typedef std::shared_ptr<query> Ptr;
	  typedef std::shared_ptr<const query> PtrConst;

	  static Ptr Create(solar::choice::choice_t c, research::data r, solar s) override;
	
	protected:
	  main_window(solar::choice::choice_t c, research::data r, solar s);
	};
      };
    };
  };
};
#endif
