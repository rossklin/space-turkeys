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
  struct solar;
  struct fleet;

  namespace client{

    class entity_selector{
      static const int max_click_distance = 20;
    public:
      bool selected;
      bool owned;
      bool area_selectable;
      sf::Color color;
      std::set<source_t> commands;
      
      entity_selector(sf::Color c, bool o);
      virtual bool contains_point(point p, float &d) = 0;
      virtual bool inside_rect(sf::FloatRect r);
      virtual void draw(window_t &w) = 0;
      virtual point get_position() = 0;
      virtual bool isa(std::string t) = 0;
      virtual std::set<idtype> get_ships() = 0;
    };

    class solar_selector : public entity_selector, public solar{
    public:

      solar_selector(solar &s, sf::Color c, bool o);
      bool contains_point(point p, float &d);
      void draw(window_t &w);
      point get_position();
      bool isa(std::string t);
      std::set<idtype> get_ships();
    };

    class fleet_selector : public entity_selector, public fleet{
    public:
      fleet_selector(fleet &f, sf::Color c, bool o);
      bool contains_point(point p, float &d);
      void draw(window_t &w);
      point get_position();
      bool isa(std::string t);
      std::set<idtype> get_ships();
    };

    class command_selector : public entity_selector, public command{
    public:
      point from;
      point to;

      command_selector(command &c, point s, point d);
      bool contains_point(point p, float &d);
      void draw(window_t &w);
      point get_position();
      bool isa(std::string t);
      std::set<idtype> get_ships();
    };
  };
};
#endif
