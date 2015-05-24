#include <string>
#include <set>

#include <SFML/Graphics.hpp>

#include "graphics.h"
#include "types.h"
#include "ship.h"

namespace st3{

  /*! command sub gui for allocating a specific ship type */
  class command_table{
    // data
    hm_t<idtype, ship> ships;
    std::vector<idtype> cache_ids;
    std::vector<idtype> alloc_ids;
    window_t *w;

    // geometry
    sf::Vector2i grid_size;
    sf::FloatRect bounds;
    float inner_padding; 
    float outer_padding;
    point symbol_dims;
    sf::FloatRect cache_bounds;
    sf::FloatRect alloc_bounds;
    int hover_index;
    int hover_side;
    sf::Color color;
    sf::ConvexShape fa_arrow;
    sf::ConvexShape fc_arrow;

    /*! move a ship between cached and allocated
      @param id id of the ship to move 
    */
    void move_ship(idtype id);

    /*! compute the table positions of a point
      @param root ul corner of the (cache or alloc) table 
      @param p the point
      @return vector of x and y table grid positions
    */
    sf::Vector2i coord2index(point root, point p);

    /*! compute the cache table grid index of a point
      @param p the point
      @return index = col + cols * row
    */
    int get_cache_index(point p);

    /*! compute the alloc table grid index of a point
      @param p the point
      @return index = col + cols * row
    */
    int get_alloc_index(point p);

  public:

    // choice
    std::set<idtype> cached, allocated; /*!< ids of waiting and ready ships */
    bool done; /*!< indicate when the interface is done */

    /*! construct a command table
      @param w window to draw on
      @param s table of available ships
      @param prealloc set of ids of initially allocated ships
      @param p point where to draw the gui
      @param c color for drawing ships 
    */
    command_table(window_t *w, hm_t<idtype, ship> s, std::set<idtype> prealloc, point p, sf::Color c);

    /*! default constructor */
    command_table();

    /*! update the gui with an event 
      @param e the event
      @return whether the event was effective
    */
    bool handle_event(sf::Event e);

    /*! draw the gui */
    void draw();

  };

  /*! gui for allocating ships to command selectors */
  class command_gui{
    std::vector<command_table> tables; /*!< one table for each ship class */
    sf::FloatRect bounds; /*!< drawing bounds in pixel coordinates */
    window_t *w; /*!< window to draw on */

    static constexpr float padding = 5;
    
  public:
    static float table_width; /*!< width of tables */
    static float table_height; /*!< height of tables */

    std::set<idtype> cached, allocated; /*!< sets of ids of waiting and allocated ships */
    idtype comid; /*!< id of associated command selector */

    /*! construct a command_gui
      @param id id of command selector to remember 
      @param w window to draw on
      @param s table of available ships
      @param prealloc ids of ships that should initially be allocated
      @param dims width and height
      @param c color for drawing ships
    */
    command_gui(idtype id, window_t *w, hm_t<idtype, ship> s, std::set<idtype> prealloc, point dims, sf::Color c);

    /*! handle an event
      @param e the event
      @return whether the event was effective 
    */
    bool handle_event(sf::Event e);

    /*! draw the gui */
    void draw();
  };
};
