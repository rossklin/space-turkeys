#ifndef _STK_SELECTOR
#define _STK_SELECTOR

#include <SFML/Graphics.hpp>
#include <string>
#include <set>

#include "graphics.h"
#include "types.h"
#include "command.h"
#include "solar.h"
#include "fleet.h"

namespace st3{
  namespace client{
    /*! base class for graphical representation of game objects */
    class entity_selector{
      static const int max_click_distance = 20; /*!< greatest distance from entities at which clicks are handled */
    public:
      static constexpr int queue_max = 100000; /*!< level at which click selection order wraps */
      static source_t last_selected; /*!< source identifier of latest selected entity */

      int queue_level; /*!< selection queue level: entities with lower level get priority */
      bool selected; /*!< is this entity selected? */
      bool owned; /*!< is the game object owned by the client? */
      bool seen; /*!< is the game object in sight for the client? */
      bool area_selectable; /*!< can this entity be area selected? */
      sf::Color color; /*!< the player color of this entity */
      std::set<idtype> commands; /*! the commands given to this entity */
      std::set<idtype> allocated_ships; /*! set of ships allocated in commands */
      
      /*! construct an entity selector with given color and ownership
	@param c the color
	@param o whether the entity is owned
      */
      entity_selector(sf::Color c, bool o);

      /*! get the color of the entity
	@return the color
      */
      sf::Color get_color();

      /*! empty destructor */
      virtual ~entity_selector();

      /*! check whether a point intersects the selector
	@param p the point
	@param[out] d the "distance" from the selector to the point
	@return whether the point intersects the selector
      */
      virtual bool contains_point(point p, float &d) = 0;

      /*! check whether the selector is inside a rectangle
	@param r the rectangle to check
	@return whether the selector is in the rectangle
      */
      virtual bool inside_rect(sf::FloatRect r);

      /*! draw the selector on a window
	@param w the window
      */
      virtual void draw(window_t &w) = 0;

      /*! get the selector's position
	@return the position
      */
      virtual point get_position() = 0;

      /*! check whether the selector is of a certain type
	@param t type identifier in st3::identifier
	@return whether the selector is of type t
      */
      virtual bool isa(std::string t) = 0;

      /*! get the ships associated to the selector
	@return set of ship ids
      */
      virtual std::set<idtype> get_ships() = 0;

      /*! get the non-allocated ships associated to the selector
	@return set of ship ids
      */
      virtual std::set<idtype> get_ready_ships();
    };

    /*! entity_selector representing a solar */
    class solar_selector : public entity_selector, public solar::solar{
    public:

      /*! construct a solar_selector with given solar, color and ownerhsip
	@param s the solar
	@param c the color
	@param o whether the selector is owned
      */
      solar_selector(st3::solar::solar &s, sf::Color c, bool o);

      /*! empty destructor */
      ~solar_selector();
      bool contains_point(point p, float &d);
      void draw(window_t &w);
      point get_position();
      bool isa(std::string t);
      std::set<idtype> get_ships();
    };

    /*! entity_selector representing a fleet */
    class fleet_selector : public entity_selector, public fleet{
    public:
      /*! construct a fleet_selector with given fleet, color and ownership
	@param f the fleet
	@param c the color
	@param o whether the client owns the selector
      */
      fleet_selector(fleet &f, sf::Color c, bool o);

      /*! empty destructor */
      ~fleet_selector();
      bool contains_point(point p, float &d);
      void draw(window_t &w);
      point get_position();
      bool isa(std::string t);
      std::set<idtype> get_ships();
    };

    /*! entity_selector representing a waypoint */
    class waypoint_selector : public entity_selector, public waypoint{
    public:
      static constexpr float radius = 10;
      std::set<idtype> ships;
      
      waypoint_selector(waypoint &f, sf::Color c);
      ~waypoint_selector();
      bool contains_point(point p, float &d);
      void draw(window_t &w);
      point get_position();
      bool isa(std::string t);
      std::set<idtype> get_ships();
    };

    class command_selector : public command{
    public:
      bool selected;
      point from;
      point to;

      command_selector(command &c, point s, point d);
      ~command_selector();
      void draw(window_t &w);
      bool contains_point(point p, float &d);
    };
  };
};
#endif
