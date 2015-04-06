/* this file defines fixed length types for network passing */ 

#ifndef _STK_TYPES
#define _STK_TYPES
#include <SFML/System.hpp>
#include <unordered_map>

namespace st3{
  typedef sf::Int32 sint;
  typedef float sfloat;
  typedef bool sbool;
  typedef sint idtype;
  typedef sint protocol_t;

  template <typename key, typename value> 
    using hm_t = std::unordered_map<key,value>;
  
  struct target_t{
    static const sint SOLAR = 0;
    static const sint FLEET = 1;
    static const sint POSITION = 2;

    sint id;
    sint type;
    point position;
  };

  typedef sf::Vector2f point;
};
#endif
