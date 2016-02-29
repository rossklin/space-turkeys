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

  typedef std::string class_t;

  typedef std::string combid;

  /*! hash map type used for different game objects */
  template <typename key, typename value>
    using hm_t = std::unordered_map<key,value>;

  /*! type used to represent a point in coordinate space */
  typedef sf::Vector2f point;

  /*! utilities for source and target identifiers */
  namespace identifier{
    const class_t solar = "solar";
    const class_t solar = "ship";
    const class_t fleet = "fleet";
    const class_t waypoint = "waypoint";
    const class_t command = "command";
    const class_t idle = "idle";
    
    const combid target_idle = "idle:0";
    const combid source_none = "noclass:noentity";

    /*! make a source symbol
      @param t type identifier
      @param i id
      @return source symbol
    */
    combid make(class_t t, idtype i);

    /*! make a source symbol with string id
      @param t type identifier
      @param k string id
      @return source symbol
    */
    combid make(class_t t, class_t k);

    /*! get the type of a source symbol
      @param s source symbol
      @return type identifier
    */
    class_t get_type(combid s);

    /*! get the id of a source symbol
      @param s source symbol with id
      @return id
    */
    idtype get_id(combid s);

    /*! get the string id of a source symbol
      @param s source symbol with string id
      @return string id
    */
    std::string get_string_id(combid s);

    /*! get the owner id from a waypoint source symbol string id
      @param v string id of a waypoint source symbol
      @return id of the owner of the waypoint
    */
    idtype get_waypoint_owner(combid v);
  };
};
#endif
