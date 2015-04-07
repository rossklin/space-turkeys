#ifndef _STK_SELECTOR
#define _STK_SELECTOR

#include <SFML/Graphics.hpp>

#include "types.h"
#include "command.h"

namespace st3{
  struct solar;
  struct fleet;

  namespace client{
    class selector{
      static const int max_click_distance = 20;
    public:
      bool selected;

      virtual bool contains_point(point p, float &d) = 0;
    };

    class entity_selector : public selector{
    public:
      bool owned;
      idtype id;
      
      entity_selector(idtype i, bool o);

      virtual source_t command_source() = 0;
      virtual bool inside_rect(sf::FloatRect r) = 0;
    };

    class solar_selector : public entity_selector{
    private:
      solar *data;

    public:
      solar_selector( solar *s, idtype i, bool o);
      bool contains_point(point p, float &d);
      source_t command_source();
      bool inside_rect(sf::FloatRect r);
    };

    class fleet_selector : public entity_selector{
    private:
      fleet *data;

    public:
      fleet_selector( fleet *s, idtype i, bool o);
      bool contains_point(point p, float &d);
      source_t command_source();
      bool inside_rect(sf::FloatRect r);
    };

    class command_selector : public selector{
    private:
      point from;
      point to;

    public:
      command c;

      command_selector(command c, point s, point d);
      bool contains_point(point p, float &d);
    };
  };
};
#endif
