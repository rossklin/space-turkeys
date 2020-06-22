#include <SFML/Graphics.hpp>
#include <set>
#include <string>

#include "graphics.h"
#include "ship.h"
#include "types.h"

namespace st3 {

/*! gui for allocating ships to command selectors */
class target_gui {
 public:
  struct option_t {
    combid key;
    std::string option;

    option_t(combid k, std::string v);
  };

  static const option_t option_cancel;
  static const option_t option_add_waypoint;

 private:
  std::vector<option_t> options;
  int highlight_index;
  window_t *window;
  sf::FloatRect bounds;

  static const point option_size;

  int compute_index(point p);

 public:
  point position;
  option_t selected_option;
  std::list<combid> selected_entities;
  bool done;

  /*! construct a target gui
      @param p coordinate position
      @param options list of target options to show
      @param sel list of selected entities to set up commands for
      @param w pointer to window for coorinate transform and drawing
    */
  target_gui(point p, std::list<option_t> options, std::list<combid> sel, window_t *w);

  /*! handle an event
      @param e the event
      @return whether the event was effective 
    */
  bool handle_event(sf::Event e);

  /*! draw the gui */
  void draw();
};
};  // namespace st3
