/* this file defines fixed length types for network passing */ 

#ifndef _STK_TYPES
#define _STK_TYPES
#include <SFML/System.hpp>
#include <unordered_map>

namespace st3{
  /*! 32 bit in for network serialization */
  typedef sf::Int32 sint;

  /*! this will later be a fixed size float for network serialization */
  typedef float sfloat;

  /*! this will later be a fixed size bool for network serialization */
  typedef bool sbool;

  /*! serializable type used for ids */
  typedef sint idtype;

  /*! serializable type used for client/server protocol */
  typedef sint protocol_t;

  /*! hash map type used for different game objects */
  template <typename key, typename value>
    using hm_t = std::unordered_map<key,value>;

  /*! type used to represent a point in coordinate space */
  typedef sf::Vector2f point;

  /*! type used to identify a game object */
  typedef std::string source_t;

  /*! type used to identify a game object target */
  typedef std::string target_t;

  /*! utilities for source and target identifiers */
  namespace identifier{
    const std::string solar = "solar";
    const std::string fleet = "fleet";
    const std::string waypoint = "waypoint";
    const std::string command = "command";
    const std::string idle = "idle";
    const std::string target_idle = "idle:0";

    /*! make a source symbol
      @param t type identifier
      @param i id
      @return source symbol
    */
    std::string make(std::string t, idtype i);

    /*! make a source symbol with string id
      @param t type identifier
      @param k string id
      @return source symbol
    */
    std::string make(std::string t, std::string k);

    
    std::string get_type(std::string s);
    st3::idtype get_id(std::string s);
    source_t get_string_id(std::string s);
    st3::idtype get_waypoint_owner(source_t v);
  };
};
#endif
