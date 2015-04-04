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
    sint id;
    sint type;
  };

  typedef sf::Vector2f point;
};
#endif
