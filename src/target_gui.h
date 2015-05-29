#include <string>
#include <set>

#include <SFML/Graphics.hpp>

#include "graphics.h"
#include "types.h"
#include "ship.h"

namespace st3{

  /*! gui for allocating ships to command selectors */
  class target_gui{
  public: 
    struct option_t{
      source_t key;
      std::string option;

      option_t(source_t k, std::string v);
    };

    static const option_t option_cancel;
    static const option_t option_add_waypoint;

  private:

    std::vector<option_t> options;
    int highlight_index;
    sf::RenderWindow *window;
    sf::FloatRect bounds;

    static const point option_size;

    int compute_index(point p);
  public:
    point position;
    option_t selected_option;
    std::list<source_t> selected_entities;
    bool done;

    /*! construct a target gui
      @param p coordinate position
      @param options list of target options to show
      @param sel list of selected entities to set up commands for
      @param w pointer to window for coorinate transform and drawing
    */
    target_gui(point p, std::list<option_t> options, std::list<source_t> sel, sf::RenderWindow *w);

    /*! handle an event
      @param e the event
      @return whether the event was effective 
    */
    bool handle_event(sf::Event e);

    /*! draw the gui */
    void draw();
  };
};
