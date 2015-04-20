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

  typedef sf::Vector2f point;
  typedef std::string source_t;
  typedef std::string target_t;
  typedef std::string shipclass_t;

  namespace identifier{
    const std::string solar = "solar";
    const std::string fleet = "fleet";
    const std::string point = "point";
    const std::string command = "command";
    const std::string shipc_scout = "shipc_scout";
    const std::string shipc_fighter = "shipc_fighter";
    const std::string none = "";

    std::string make(std::string t, idtype i);
    std::string make(std::string t, st3::point p);
    std::string get_type(std::string s);
    st3::idtype get_id(std::string s);
    st3::point get_point(std::string s);
  };
};
#endif
