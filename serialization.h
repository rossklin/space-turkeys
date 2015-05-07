#ifndef _STK_SERIALIZATION
#define _STK_SERIALIZATION

#include "game_data.h"

namespace st3{

  // stream ops for lists
  template<typename T>
    sf::Packet& operator <<(sf::Packet& packet, const T &container);
  template<typename T>
    sf::Packet& operator >>(sf::Packet& packet, T &container);

  // stream ops for hm_t
  template<typename ID, typename T>
    sf::Packet& operator <<(sf::Packet& packet, const hm_t<ID, T> &g);
  template<typename ID, typename T>
    sf::Packet& operator >>(sf::Packet& packet, hm_t<ID, T> &g);

  // game data structs
  sf::Packet& operator <<(sf::Packet& packet, const game_data &g);
  sf::Packet& operator >>(sf::Packet& packet, game_data &g);

  sf::Packet& operator <<(sf::Packet& packet, const game_settings &g);
  sf::Packet& operator >>(sf::Packet& packet, game_settings &g);

  // choice structs
  sf::Packet& operator <<(sf::Packet& packet, const choice &c);
  sf::Packet& operator >>(sf::Packet& packet, choice &c);

  sf::Packet& operator <<(sf::Packet& packet, const waypoint &c);
  sf::Packet& operator >>(sf::Packet& packet, waypoint &c);

  sf::Packet& operator <<(sf::Packet& packet, const command &c);
  sf::Packet& operator >>(sf::Packet& packet, command &c);

  // ship
  sf::Packet& operator <<(sf::Packet& packet, const ship &g);
  sf::Packet& operator >>(sf::Packet& packet, ship &g);

  // solar
  sf::Packet& operator <<(sf::Packet& packet, const solar::solar &g);
  sf::Packet& operator >>(sf::Packet& packet, solar::solar &g);

  // solar choice
  sf::Packet& operator <<(sf::Packet& packet, const solar::choice_t &g);
  sf::Packet& operator >>(sf::Packet& packet, solar::choice_t &g);

  // fleet
  sf::Packet& operator <<(sf::Packet& packet, const fleet &g);
  sf::Packet& operator >>(sf::Packet& packet, fleet &g);

  // point
  sf::Packet& operator <<(sf::Packet& packet, const point &g);
  sf::Packet& operator >>(sf::Packet& packet, point &g);

  // player
  sf::Packet& operator <<(sf::Packet& packet, const player &g);
  sf::Packet& operator >>(sf::Packet& packet, player &g);

  // player
  sf::Packet& operator <<(sf::Packet& packet, const research &g);
  sf::Packet& operator >>(sf::Packet& packet, research &g);
};
#endif
