#pragma once

#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <list>
#include <memory>
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

/*! hash map type used for different game objects */
template <typename key, typename value>
using hm_t = std::unordered_map<key, value>;

/*! type used to represent a point in coordinate space */
typedef sf::Vector2f point;
typedef std::vector<point> path_t;

struct id_pair {
  idtype a;
  idtype b;

  id_pair(idtype x, idtype y);
};

bool operator<(const id_pair &x, const id_pair &y);

// declare classes and pointer types
class ship;
class solar;
class fleet;
class waypoint;
class game_object;
class physical_object;
class commandable_object;

namespace server {
struct server_cl_socket;
};

typedef std::shared_ptr<ship> ship_ptr;
typedef std::shared_ptr<solar> solar_ptr;
typedef std::shared_ptr<fleet> fleet_ptr;
typedef std::shared_ptr<waypoint> waypoint_ptr;
typedef std::shared_ptr<game_object> game_object_ptr;
typedef std::shared_ptr<physical_object> physical_object_ptr;
typedef std::shared_ptr<commandable_object> commandable_object_ptr;

typedef std::shared_ptr<server::server_cl_socket> server_cl_socket_ptr;
typedef std::shared_ptr<sf::Packet> packet_ptr;

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
const class_t noclass = "noclass";

const idtype no_entity = -1;

};  // namespace identifier
};  // namespace st3
