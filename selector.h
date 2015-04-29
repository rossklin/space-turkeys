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

    class entity_selector{
      static const int max_click_distance = 20;
    public:
      bool selected;
      bool owned;
      bool area_selectable;
      sf::Color color;
      std::set<idtype> commands;
      std::set<idtype> allocated_ships;
      
      entity_selector(sf::Color c, bool o);
      virtual ~entity_selector();
      virtual bool contains_point(point p, float &d) = 0;
      virtual bool inside_rect(sf::FloatRect r);
      virtual void draw(window_t &w) = 0;
      virtual point get_position() = 0;
      virtual bool isa(std::string t) = 0;
      virtual std::set<idtype> get_ships() = 0;
      virtual std::set<idtype> get_ready_ships();
    };

    class solar_selector : public entity_selector, public solar::solar{
    public:

      solar_selector(st3::solar::solar &s, sf::Color c, bool o);
      ~solar_selector();
      bool contains_point(point p, float &d);
      void draw(window_t &w);
      point get_position();
      bool isa(std::string t);
      std::set<idtype> get_ships();
    };

    class fleet_selector : public entity_selector, public fleet{
    public:
      fleet_selector(fleet &f, sf::Color c, bool o);
      ~fleet_selector();
      bool contains_point(point p, float &d);
      void draw(window_t &w);
      point get_position();
      bool isa(std::string t);
      std::set<idtype> get_ships();
    };

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
