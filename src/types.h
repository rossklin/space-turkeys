/* this file defines fixed length types for network passing */

#ifndef _STK_TYPES
#define _STK_TYPES
#include <SFML/System.hpp>
#include <list>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace st3 {
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
void log(std::string v, std::string severity = "notice");
};  // namespace server

extern const float accuracy_distance_norm;
extern const float evasion_mass_norm;

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
using hm_t = std::unordered_map<key, value>;

/*! type used to represent a point in coordinate space */
typedef sf::Vector2f point;
typedef std::list<point> path_t;

struct id_pair {
  combid a;
  combid b;

  id_pair(combid x, combid y);
};

struct terrain_object {
  class_t type;
  point center;
  std::vector<point> border;

  std::pair<int, int> intersects_with(terrain_object b, float r = 0);
  point get_vertice(int idx, float rbuf = 0) const;
  void set_vertice(int idx, point p);
  int triangle(point p, float r) const;
  std::vector<point> get_border(float r) const;
  point closest_exit(point p, float r) const;
};

bool operator<(const id_pair &x, const id_pair &y);

namespace keywords {
extern const std::vector<std::string> resource;
extern const std::vector<std::string> development;
extern const std::vector<std::string> solar_modifier;

// resources
const std::string key_metals = "metals";
const std::string key_organics = "organics";
const std::string key_gases = "gases";

// developments
const std::string key_research = "research facility";
const std::string key_shipyard = "shipyard";
const std::string key_development = "development";
const std::string key_agriculture = "agriculture";
const std::string key_defense = "defense";

// other solar modifiers
const std::string key_medicine = "medicine";
const std::string key_population = "population";

// disabled
const std::string build_disabled = "disabled";
};  // namespace keywords

/*! utilities for source and target identifiers */
namespace identifier {
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

idtype get_multid_index(combid v);
std::string get_multid_index_symbol(combid v);

combid make_waypoint_id(idtype owner, idtype id);
};  // namespace identifier
};  // namespace st3
#endif
