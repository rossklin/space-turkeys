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

  template <typename key, typename value, typename... params> 
    using hm_t = std::unordered_map<key,value,params...>;

  typedef sf::Vector2f point;
  
  struct source_t{
    static const sint SOLAR = 0;
    static const sint FLEET = 1;

    struct hash{
      size_t operator()(const source_t &a) const;
    };

    struct pred{
      bool operator()(const source_t &a, const source_t &b) const;
    };

    sint id;
    sint type;
  };

  struct target_t : public source_t{
    static const sint POINT = 2;
    point position;
  };
};
#endif
