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
      static const int queue_max = 100000; /*!< level at which click selection order wraps */
      static source_t last_selected; /*!< source identifier of latest selected entity */

      int queue_level; /*!< selection queue level: entities with lower level get priority */
      bool selected; /*!< is this entity selected? */
      bool owned; /*!< is the game object owned by the client? */
      bool seen; /*!< is the game object in sight for the client? */
      bool area_selectable; /*!< can this entity be area selected? */
      sf::Color color; /*!< the player color of this entity */
      std::set<idtype> commands; /*! the commands given to this entity */
      
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
      static constexpr float radius = 10; /*!< grid size of waypoint representation */
      std::set<idtype> ships; /*!< ids of ships available at this waypoint */
      
      /*! construct a waypoint selector with given waypoint and color
	@param w the waypoint
	@param c the color
       */
      waypoint_selector(waypoint &w, sf::Color c);

      /*! empty deconstructor */
      ~waypoint_selector();
      bool contains_point(point p, float &d);
      void draw(window_t &w);
      point get_position();
      bool isa(std::string t);
      std::set<idtype> get_ships();
    };

    /*! selector representing a command */
    class command_selector : public command{
    public:
      static const int queue_max = 100000; /*!< level at which click selection order wraps */
      static idtype last_selected;
      int queue_level; /*!< selection queue level: command selectors with lower level get priority */
      bool selected; /*!< whether the command_selector is selected */
      point from; /*!< source point */
      point to; /*!< destination point */

      /*! construct a command_selector with given command, source point and destination point 
	@param c the command
	@param s the source point
	@param d the destination point
      */
      command_selector(command &c, point s, point d);

      /*! empty destructor */
      ~command_selector();

      /*! draw the command selector on a window
	@param w the window
      */
      void draw(window_t &w);

      /*! check whether a point intersects the command_selector
	@param p the point
	@param[out] d the "distance" from the selector to the point
	@return whether the point intersects the command_selector
      */
      bool contains_point(point p, float &d);
    };
  };
};
#endif
