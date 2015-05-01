#include <string>
#include <set>

#include <SFML/Graphics.hpp>

#include "types.h"
#include "ship.h"

namespace st3{
  class command_table{
    // data
    hm_t<idtype, ship> ships;
    std::vector<idtype> cache_ids;
    std::vector<idtype> alloc_ids;
    sf::RenderWindow *w;

    // geometry
    sf::Vector2i grid_size;
    sf::FloatRect bounds;
    float inner_padding;
    float outer_padding;
    point symbol_dims;
    sf::FloatRect cache_bounds;
    sf::FloatRect alloc_bounds;
    int hover_index;
    int hover_side;
    sf::Color color;
    sf::ConvexShape fa_arrow;
    sf::ConvexShape fc_arrow;

  public:

    // choice
    std::set<idtype> cached, allocated;
    bool done;

    command_table(sf::RenderWindow *w, hm_t<idtype, ship> s, std::set<idtype> prealloc, point p, sf::Color c);
    command_table();
    void move_ship(idtype id);
    bool handle_event(sf::Event e);
    void draw();
    sf::Vector2i coord2index(point root, point p);
    int get_cache_index(point p);
    int get_alloc_index(point p);
  };

  class command_gui{
    std::vector<command_table> tables;
    sf::FloatRect bounds;
    sf::RenderWindow *w;
    
  public:
    static constexpr float padding = 5;
    static constexpr float table_width = 200;
    static constexpr float table_height = 100;

    std::set<idtype> cached, allocated;
    idtype comid;

    command_gui(idtype id, sf::RenderWindow *w, hm_t<idtype, ship> s, std::set<idtype> prealloc, point p, sf::Color c);
    bool handle_event(sf::Event e);
    void draw();
  };
};
