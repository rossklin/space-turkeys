#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <set>
#include <string>

#include "command.hpp"
#include "fleet.hpp"
#include "game_object.hpp"
#include "graphics.hpp"
#include "rsg/src/RskTypes.hpp"
#include "ship.hpp"
#include "solar.hpp"
#include "types.hpp"
#include "waypoint.hpp"

namespace st3 {
struct game;

/*! base class for graphical representation of game objects */
class entity_selector : public virtual game_object {
  static const int max_click_distance = 20; /*!< greatest distance from entities at which clicks are handled */
 public:
  typedef std::shared_ptr<entity_selector> ptr;

  int queue_level;           /*!< selection queue level: entities with lower level get priority */
  bool selected;             /*!< is this entity selected? */
  bool owned;                /*!< is the game object owned by the client? */
  bool seen;                 /*!< is the game object in sight for the client? */
  sf::Color color;           /*!< the player color of this entity */
  std::set<idtype> commands; /*! the commands given to this entity */
  point base_position;
  float base_angle;

  entity_selector(sf::Color c, bool o);
  virtual ~entity_selector() = default;
  virtual ptr ss_clone() = 0;

  virtual bool contains_point(point p, float &d) = 0;
  virtual void draw(RSG::WindowPtr w) = 0;
  virtual point get_position() = 0;
  virtual std::set<combid> get_ships() = 0;
  virtual std::string hover_info() = 0;
  hm_t<std::string, int> ship_counts();

  sf::Color get_color();
  virtual bool inside_rect(sf::FloatRect r);
  virtual bool is_selectable() = 0;
  virtual bool is_area_selectable();
};

/*! entity_selector representing a specific object class */
template <typename T>
class specific_selector : public virtual entity_selector, public virtual T {
 public:
  typedef T base_object_t;
  typedef std::shared_ptr<specific_selector<T>> ptr;
  static ptr create(T &s, sf::Color c, bool o);

  specific_selector(T &s, sf::Color c, bool o);
  ~specific_selector() = default;
  entity_selector::ptr ss_clone();
  bool is_selectable();
  bool contains_point(point p, float &d);
  void draw(RSG::WindowPtr w);
  point get_position();
  std::set<combid> get_ships();
  std::string hover_info();
};

// template <>
// specific_selector<ship>::specific_selector(ship &s, sf::Color c, bool o);

typedef specific_selector<ship> ship_selector;
typedef specific_selector<fleet> fleet_selector;
typedef specific_selector<solar> solar_selector;
typedef specific_selector<waypoint> waypoint_selector;

/*! selector representing a command */
class command_selector : public command {
 public:
  typedef std::shared_ptr<command_selector> ptr;

  static ptr create(command c, point f, point t);

  idtype id;
  int queue_level; /*!< selection queue level: command selectors with lower level get priority */
  bool selected;   /*!< whether the command_selector is selected */
  point from;      /*!< source point */
  point to;        /*!< destination point */

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
  void draw(RSG::WindowPtr w);

  /*! check whether a point intersects the command_selector
	@param p the point
	@param[out] d the "distance" from the selector to the point
	@return whether the point intersects the command_selector
      */
  bool contains_point(point p, float &d);
};
};  // namespace st3
