#ifndef _STK_SERIALIZATION
#define _STK_SERIALIZATION

#include "research.h"
#include "development_tree.h"
#include "game_data.h"
#include "cost.h"

namespace st3{

  /* **************************************** */
  /*   CONTAINERS */
  /* **************************************** */

  /*! stream a list-like container into a packet
    @param packet the packet
    @param container the container
    @return reference to the resulting packet
  */
  template<typename T>
  sf::Packet& inner_iterated_insert(sf::Packet& packet, const T &container);

  /*! stream a list-like container out of a packet
    @param packet the packet
    @param container the container
    @return reference to the resulting packet
  */
  template<typename T>
  sf::Packet& inner_iterated_extract(sf::Packet& packet, T &container);

  // testing container specific templates to avoid selection ambiguity
  template<typename T>
  sf::Packet& operator <<(sf::Packet& packet, const std::list<T> &container);
  template<typename T>
  sf::Packet& operator >>(sf::Packet& packet, std::list<T> &container);

  template<typename T>
  sf::Packet& operator <<(sf::Packet& packet, const std::vector<T> &container);
  template<typename T>
  sf::Packet& operator >>(sf::Packet& packet, std::vector<T> &container);

  template<typename T>
  sf::Packet& operator <<(sf::Packet& packet, const std::set<T> &container);
  template<typename T>
  sf::Packet& operator >>(sf::Packet& packet, std::set<T> &container);

  // stream ops for hm_t
  /*! stream a hm_t into a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  template<typename ID, typename T>
  sf::Packet& operator <<(sf::Packet& packet, const hm_t<ID, T> &g);

  /*! stream a hm_t out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  template<typename ID, typename T>
  sf::Packet& operator >>(sf::Packet& packet, hm_t<ID, T> &g);

  template<typename F, typename S>
  sf::Packet& operator <<(sf::Packet& packet, const std::pair<F,S> &g);
  
  template<typename F, typename S>
  sf::Packet& operator >>(sf::Packet& packet, std::pair<F,S> &g);

  /* **************************************** */
  /*   GAME DATA OBJECTS */
  /* **************************************** */

  sf::Packet& operator <<(sf::Packet& packet, const cost::allocation &g);
  sf::Packet& operator >>(sf::Packet& packet, cost::allocation &g);

  sf::Packet& operator <<(sf::Packet& packet, const entity_package &g);

  sf::Packet& operator << (sf::Packet& packet, const commandable_object &g);
  sf::Packet& operator >> (sf::Packet& packet, commandable_object &g);

  sf::Packet& operator << (sf::Packet& packet, const game_object &g);
  sf::Packet& operator >> (sf::Packet& packet, game_object &g);

  /*! stream a game_settings into a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const game_settings &g);

  /*! stream a game_settings out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, game_settings &g);

  // choice structs

  /*! stream a choice into a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const choice::choice &c);

  /*! stream a choice out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, choice::choice &c);

  /*! stream a waypoint into packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const waypoint &c);

  /*! stream a waypoint out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, waypoint &c);

  /*! stream a command into packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const command &c);

  /*! stream a command out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, command &c);

  sf::Packet& operator <<(sf::Packet& packet, const target_condition &c);
  sf::Packet& operator >>(sf::Packet& packet, target_condition &c);

  sf::Packet& operator <<(sf::Packet& packet, const ship_stats &s);
  sf::Packet& operator >>(sf::Packet& packet, ship_stats &s);

  /*! stream a ship into packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const ship &g);

  /*! stream a ship out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, ship &g);

  /*! stream a turret into packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const turret_t &g);

  /*! stream a turret out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, turret_t &g);

  sf::Packet& operator <<(sf::Packet& packet, const facility &g);
  sf::Packet& operator >>(sf::Packet& packet, facility &g);

  sf::Packet& operator <<(sf::Packet& packet, const facility_object &g);
  sf::Packet& operator >>(sf::Packet& packet, facility_object &g);

  sf::Packet& operator <<(sf::Packet& packet, const development::node &g);
  sf::Packet& operator >>(sf::Packet& packet, development::node &g);

  /*! stream a solar into packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const solar &g);

  /*! stream a solar out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, solar &g);

  /*! stream a solar choice into packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const choice::c_solar &g);

  /*! stream a solar choice out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, choice::c_solar &g);

  /*! stream a fleet into packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const fleet &g);

  /*! stream a fleet out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, fleet &g);

  sf::Packet& operator <<(sf::Packet& packet, const fleet::analytics &g);
  sf::Packet& operator >>(sf::Packet& packet, fleet::analytics &g);

  /*! stream a point into packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const point &g);

  /*! stream a point out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, point &g);

  /*! stream a player into a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const player &g);

  /*! stream a player out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, player &g);

  /*! stream a research into a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const research::data &g);

  /*! stream a research out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, research::data &g);

  /*! stream a research tech into a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const research::tech &g);

  /*! stream a research tech out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, research::tech &g);
};
#endif
