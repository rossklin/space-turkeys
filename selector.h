#ifndef _STK_SELECTOR
#define _STK_SELECTOR

#include <SFML/Graphics.hpp>

#include "types.h"
#include "command.h"

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
      idtype id;
      point position;
      
      entity_selector(idtype i, bool o, point p);
      virtual bool contains_point(point p, float &d) = 0;
      virtual source_t command_source() = 0;
      virtual bool inside_rect(sf::FloatRect r);
    };

    class solar_selector : public entity_selector{
    private:
      float radius;

    public:
      solar_selector(idtype i, bool o, point p, float r);
      bool contains_point(point p, float &d);
      source_t command_source();
    };

    class fleet_selector : public entity_selector{
    private:
      float radius;

    public:
      fleet_selector(idtype i, bool o, point p, float r);
      bool contains_point(point p, float &d);
      source_t command_source();
    };

    class command_selector : public entity_selector{
    private:
      point from;

    public:

      command_selector(idtype i, point s, point d);
      bool contains_point(point p, float &d);
      source_t command_source();
      point get_to();
      point get_from();
    };
  };
};
#endif
