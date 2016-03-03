#ifndef _STK_SERIALIZATION
#define _STK_SERIALIZATION

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

  template<typename T>
  sf::Packet& operator <<(sf::Packet& packet, const cost::allocation<T> &g);

  template<typename T>
  sf::Packet& operator >>(sf::Packet& packet, cost::allocation<T> &g);


  /* **************************************** */
  /*   GAME DATA OBJECTS */
  /* **************************************** */

  /*! stream a hm_t into a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const game_data &g);

  /*! stream a hm_t out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, game_data &g);

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

  sf::Packet& operator <<(sf::Packet& packet, const interaction::target_condition &c);
  sf::Packet& operator >>(sf::Packet& packet, interaction::target_condition &c);

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
  sf::Packet& operator <<(sf::Packet& packet, const turret &g);

  /*! stream a turret out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, turret &g);

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

  /*! stream a resource_data into a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const cost::resource_data &g);

  /*! stream a resource_data out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, cost::resource_data &g);
};
#endif
