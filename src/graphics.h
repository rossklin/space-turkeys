#ifndef _STK_GRAPHICS
#define _STK_GRAPHICS

#include <string>
#include <functional>

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

    
    /* **************************************** */
    /* SFML STUFF */
    /* **************************************** */

    /*! build and sfml rectangle shape from given bounds
      @param r bounds
      @return an sf::RectangleShape with size and position from r
    */
    sf::RectangleShape build_rect(sf::FloatRect r);

    /*! compute coordinates of the upper left corner of the current view
      @param w window to get the view from
      @return coordinates corresponding to UL corner of current view of w
    */
    point ul_corner(window_t &w);

    /*! compute an sf::Transform mapping from coordinates to pixels
      @param w the current window
      @return the transform
    */
    sf::Transform view_inverse_transform(window_t &w);

    /*! compute the scale factor coordinates per pixel

      such that coord_dims = inverse_scale() * pixel_dims
      
      @param w the current window
      @return point containing x and y scales
    */
    point inverse_scale(window_t &w);

    namespace interface{

      // base class for choice windows
      template<typename C, typename R>
      class query : public C{
      public:
	R response;
	query(char style = 0);
      };

      // bottom panel
      class bottom_panel : public sfg::Window{
      public:
	typedef std::shared_ptr<bottom_panel> Ptr;
	typedef std::shared_ptr<const bottom_panel> PtrConst;

	static Ptr Create(bool &a, bool &b);
	
      protected:	
	bottom_panel();	
      };

      // top panel
      class top_panel : public sfg::Window{
      public:
	typedef std::shared_ptr<top_panel> Ptr;
	typedef std::shared_ptr<const top_panel> PtrConst;

	static Ptr Create();
	
      protected:
	top_panel();	
      };

      // research window
      class research_window : public query<sfg::Window, choice::c_research>{
      public:
	typedef std::shared_ptr<research_window> Ptr;
	typedef std::shared_ptr<const research_window> PtrConst;

	static Ptr Create(choice::c_research *c);
	
      protected:
	research_window(choice::c_research *c);
      };

      // solar choice windows
      class main_window : public query<sfg::Window, choice::c_solar>{
      public:
	typedef std::shared_ptr<main_window> Ptr;
	typedef std::shared_ptr<const main_window> PtrConst;

	static const std::string sfg_id;

	static Ptr Create(solar::ptr s);
      protected:
	// sub interface tracker
	sfg::Box::Ptr layout;
	sfg::Box::Ptr choice_layout;
	sfg::Box::Ptr sub_layout;
	sfg::Box::Ptr info_layout;
	sfg::Label::Ptr tooltip;

	solar::ptr sol;

	main_window(solar::ptr s);
	void build_info();
	void build_choice();
	sfg::Box::Ptr new_sub(std::string v);
	void build_military();
	void build_mining();
	void build_expansion();

	sfg::Button::Ptr priority_button(std::string label, float &data, std::function<bool()> inc_val, sfg::Label::Ptr tip = 0);
      };

      // desktop geometry data
      extern sf::Vector2u desktop_dims;
      extern sf::FloatRect qw_allocation;
      extern int top_height;
      extern int bottom_start;

      // main interface
      class main_interface : public sfg::Desktop {
      public:
	sfg::Widget::Ptr query_window;
	sfg::Label::Ptr hover_label;

	// research level used by interface components
	research::data &research_level;

	// data for generating the client's choice
	choice::choice response;

	// progress communication variables for game loop
	bool accept;
	bool done;

	main_interface(sf::Vector2u dims, research::data &r);
	void reset_qw(sfg::Widget::Ptr p);
	void clear_qw();
      };

      extern main_interface *desktop;
    };
  };
};
#endif
