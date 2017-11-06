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

    extern const sf::Color solar_neutral; /*!< color for neutral solars */
    extern const sf::Color command_selected_head;/*!< color for selected command head */
    extern const sf::Color command_selected_body;/*!< color for selected command body */
    extern const sf::Color command_selected_tail;/*!< color for selected command tail */
    extern const sf::Color command_selected_text;/*!< color for selected command text */
    extern const sf::Color command_normal_head;/*!< color for normal command head */
    extern const sf::Color command_normal_body;/*!< color for normal command body */
    extern const sf::Color command_normal_tail;/*!< color for normal command tail */
    extern const sf::Color command_normal_text;/*!< color for normal command text */
    extern const sf::Color solar_fill; /*!< color for solar interior */
    extern const sf::Color solar_selected; /*!< color for selected solar border */
    extern const sf::Color solar_selected_fill; /*!< color for selected solar interior */
    extern const sf::Color fleet_fill; /*!< color for fleet interior */
    extern const sf::Color fleet_outline; /*!< color for fleet border */
    extern const float ship_scale_factor;

    float unscale();

    /*! compute a linear interpolation between two colors
      @param from first color
      @param to second color
      @param r proportion between from and to
      @return sf::Color "from + r * (to - from)"
    */
    sf::Color fade_color(sf::Color from, sf::Color to, float r);

    /*! initialize static graphics i.e. default font */
    void initialize();
    void draw_flag(sf::RenderTarget &w, point p, sf::Color c, sf::Color bg, int count, std::string ship_class, int nstack = 1);
    void draw_text(sf::RenderTarget &w, std::string v, point p, float fs, bool ul = false, sf::Color fill = sf::Color::White, bool do_inv = true, float rotate = 0);
    void draw_framed_text(sf::RenderTarget &w, std::string v, sf::FloatRect r, sf::Color co, sf::Color cf = sf::Color::Transparent, float fs = 0);

    void draw_circle(sf::RenderTarget &w, point p, float r, sf::Color co, sf::Color cf = sf::Color::Transparent, float b = -1);

    /*! draw a ship to a window 
      @param w window
      @param s ship
      @param c color
      @param sc scale
    */
    void draw_ship(sf::RenderTarget &w, ship s, sf::Color c, float sc = 1, bool multicolor = true);

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

    sf::Image selector_card(std::string title, bool available, float progress);
    void draw_frame(sf::FloatRect r, int thickness, sf::Color co, sf::Color cf = sf::Color::Transparent); 
    sf::RectangleShape build_rect(sf::FloatRect bounds, float thickness = -1, sf::Color co = sf::Color::White, sf::Color cf = sf::Color::Transparent);
    sfg::Widget::Ptr wrap_in_scroll(sfg::Widget::Ptr w, bool horizontal, int dim);
    sfg::Widget::Ptr wrap_in_scroll2(sfg::Widget::Ptr w, int width, int height);
 };
};
#endif
