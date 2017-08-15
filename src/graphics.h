#ifndef _STK_GRAPHICS
#define _STK_GRAPHICS

#include <string>
#include <functional>
#include <SFML/Graphics.hpp>
#include <SFGUI/Widgets.hpp>

#include "game_data.h"
#include "choice.h"
#include "types.h"
#include "ship.h"
#include "research.h"
#include "animation.h"

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
    const float ship_scale_factor = 0.1;

    /*! compute a linear interpolation between two colors
      @param from first color
      @param to second color
      @param r proportion between from and to
      @return sf::Color "from + r * (to - from)"
    */
    sf::Color fade_color(sf::Color from, sf::Color to, float r);

    /*! initialize static graphics i.e. default font */
    void initialize();
    void draw_flag(sf::RenderTarget &w, point p, float s, sf::Color c);
    void draw_text(sf::RenderTarget &w, std::string v, point p, int fs, bool ul = false, sf::Color fill = sf::Color::White);
    void draw_framed_text(sf::RenderTarget &w, std::string v, sf::FloatRect r, sf::Color co, sf::Color cf = sf::Color::Transparent);

    void draw_circle(sf::RenderTarget &w, point p, float r, sf::Color co, sf::Color cf = sf::Color::Transparent, float b = -1);

    /*! draw a ship to a window 
      @param w window
      @param s ship
      @param c color
      @param sc scale
    */
    void draw_ship(sf::RenderTarget &w, ship s, sf::Color c, float sc = 1);

    sfg::Button::Ptr ship_button(std::string ship_class, float width, float height, sf::Color col = sf::Color::Green);
    sf::Image ship_image(std::string ship_class, float width, float height, sf::Color col = sf::Color::Green);
    sf::Image ship_image_label(std::string text, std::string ship_class, float width, float height, sf::Color l_col = sf::Color::White, sf::Color s_col = sf::Color::Green);

    void draw_animation(sf::RenderTarget &w, animation e);
    
    /* **************************************** */
    /* SFML STUFF */
    /* **************************************** */

    /*! compute coordinates of the upper left corner of the current view
      @param w window to get the view from
      @return coordinates corresponding to UL corner of current view of w
    */
    point ul_corner(sf::RenderTarget &w);

    /*! compute an sf::Transform mapping from coordinates to pixels
      @param w the current window
      @return the transform
    */
    sf::Transform view_inverse_transform(sf::RenderTarget &w);

    /*! compute the scale factor coordinates per pixel

      such that coord_dims = inverse_scale() * pixel_dims
      
      @param w the current window
      @return point containing x and y scales
    */
    point inverse_scale(sf::RenderTarget &w);

    sf::Image selector_card(std::string title, bool available, float progress, std::list<std::string> info = {}, std::list<std::string> requirements = {});
    void draw_frame(sf::FloatRect r, int thickness, sf::Color co, sf::Color cf = sf::Color::Transparent); 
    sf::RectangleShape build_rect(sf::FloatRect bounds, int thickness = -1, sf::Color co = sf::Color::White, sf::Color cf = sf::Color::Transparent);
    sfg::Widget::Ptr wrap_in_scroll(sfg::Widget::Ptr w, bool horizontal, int dim);
    sfg::Widget::Ptr wrap_in_scroll2(sfg::Widget::Ptr w, int width, int height);
 };
};
#endif
