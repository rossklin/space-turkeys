/* this file defines fixed length types for network passing */ 

#ifndef _STK_TYPES
#define _STK_TYPES
#include <vector>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <SFML/System.hpp>

namespace st3{
  class classified_error : public std::runtime_error {
  public:
    std::string severity;
    classified_error(std::string v, std::string severity = "notice");
  };
  
  class logical_error : public classified_error {
  public:
    logical_error(std::string v, std::string severity = "logic-error");
  };
  
  class player_error : public classified_error {
  public:
    player_error(std::string v, std::string severity = "player-input");
  };
  
  class parse_error : public classified_error {
  public:
    parse_error(std::string v, std::string severity = "parse");
  };
  
  class network_error : public classified_error {
  public:
    network_error(std::string v, std::string severity = "network");
  };
  
  namespace server {
    extern bool silent_mode;
    void output(std::string v, bool force = false);
  };

  const float accuracy_distance_norm = 30;
  
  /*! 32 bit int for network serialization */
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

  struct id_pair {
    combid a;
    combid b;

    id_pair(combid x, combid y);
  };
  
  struct terrain_object {
    class_t type;
    point center;
    std::vector<point> border;

    std::pair<int, int> intersects_with(terrain_object b);
    point get_vertice(int idx) const;
    void set_vertice(int idx, point p);
    int triangle(point p, float r);
  };

  bool operator<(const id_pair &x, const id_pair &y);

  namespace keywords{
    extern const std::vector<std::string> resource;
    extern const std::vector<std::string> sector;
    extern const std::vector<std::string> governor;

    const std::string key_metals = "metals";
    const std::string key_organics = "organics";
    const std::string key_gases = "gases";
    const std::string key_research = "research";
    const std::string key_development = "development";
    const std::string key_culture = "culture";
    const std::string key_mining = "mining";
    const std::string key_military = "military";
    const std::string key_medicine = "medicine";
    const std::string key_ecology = "ecology";
  };

  /*! utilities for source and target identifiers */
  namespace identifier{
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

    /*! get the owner id from a waypoint source symbol string id
      @param v string id of a waypoint source symbol
      @return id of the owner of the waypoint
    */
    idtype get_multid_owner(combid v);
    std::string get_multid_owner_symbol(combid v);

    combid make_waypoint_id(idtype owner, idtype id);
  };
};
#endif
