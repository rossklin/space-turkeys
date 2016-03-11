#ifndef _STK_SELECTOR
#define _STK_SELECTOR

#include <SFML/Graphics.hpp>
#include <string>
#include <set>
#include <memory>

#include "game_object.h"
#include "graphics.h"
#include "types.h"
#include "command.h"
#include "solar.h"
#include "fleet.h"

namespace st3{
  namespace client{
    /*! base class for graphical representation of game objects */
    class entity_selector : public virtual game_object{
      static const int max_click_distance = 20; /*!< greatest distance from entities at which clicks are handled */
    public:
      typedef std::shared_ptr<entity_selector> ptr;
      
      int queue_level; /*!< selection queue level: entities with lower level get priority */
      bool selected; /*!< is this entity selected? */
      bool owned; /*!< is the game object owned by the client? */
      bool seen; /*!< is the game object in sight for the client? */
      bool area_selectable; /*!< can this entity be area selected? */
      sf::Color color; /*!< the player color of this entity */
      std::set<idtype> commands; /*! the commands given to this entity */
      
      entity_selector(sf::Color c, bool o);
      virtual ~entity_selector();

      sf::Color get_color();
      virtual bool contains_point(point p, float &d) = 0;
      virtual bool inside_rect(sf::FloatRect r);
      virtual void draw(window_t &w) = 0;
      virtual point get_position() = 0;
      virtual bool isa(std::string t) = 0;
      virtual std::set<combid> get_ships() = 0;
      virtual std::string hover_info() = 0;
    };

    /*! entity_selector representing a specific object class */
    template <typename T>
    class specific_selector : public virtual entity_selector, public virtual T{
    public:
      typedef T base_object_t;
      typedef std::shared_ptr<specific_selector<T> > ptr;
      static ptr create(T &s, sf::Color c, bool o);

      specific_selector(T &s, sf::Color c, bool o);

      /*! empty destructor */
      ~specific_selector();
      bool contains_point(point p, float &d);
      void draw(window_t &w);
      point get_position();
      bool isa(std::string t);
      std::set<combid> get_ships();
      std::string hover_info();
    };

    typedef specific_selector<ship> ship_selector;
    typedef specific_selector<fleet> fleet_selector;
    typedef specific_selector<solar> solar_selector;
    typedef specific_selector<waypoint> waypoint_selector;

    /*! selector representing a command */
    class command_selector : public command{
    public:
      typedef std::shared_ptr<command_selector> ptr;

      static ptr create(command c, point f, point t);
      
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
