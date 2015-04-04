#ifndef _STK_SERIALIZATION
#define _STK_SERIALIZATION

#include "game_data.h"

namespace st3{
  // stream ops for hm_t
  template<typename T>
    sf::Packet& operator <<(sf::Packet& packet, const hm_t<idtype, T> &g);
  template<typename T>
    sf::Packet& operator >>(sf::Packet& packet, hm_t<idtype, T> &g);

  /* // stream ops for lists */
  /* template<typename T> */
  /*   sf::Packet& operator <<(sf::Packet& packet, const std::list<T> &container); */
  /* template<typename T> */
  /*   sf::Packet& operator >>(sf::Packet& packet, std::list<T> &container); */

  // game data structs
  sf::Packet& operator <<(sf::Packet& packet, const game_data &g);
  sf::Packet& operator >>(sf::Packet& packet, game_data &g);

  sf::Packet& operator <<(sf::Packet& packet, const game_settings &g);
  sf::Packet& operator >>(sf::Packet& packet, game_settings &g);

  // choice structs
  sf::Packet& operator <<(sf::Packet& packet, const choice &c);
  sf::Packet& operator >>(sf::Packet& packet, choice &c);

  sf::Packet& operator <<(sf::Packet& packet, const command &c);
  sf::Packet& operator >>(sf::Packet& packet, command &c);

  sf::Packet& operator <<(sf::Packet& packet, const target_t &c);
  sf::Packet& operator >>(sf::Packet& packet, target_t &c);

  // ship
  sf::Packet& operator <<(sf::Packet& packet, const ship &g);
  sf::Packet& operator >>(sf::Packet& packet, ship &g);

  // solar
  sf::Packet& operator <<(sf::Packet& packet, const solar &g);
  sf::Packet& operator >>(sf::Packet& packet, solar &g);

  // point
  sf::Packet& operator <<(sf::Packet& packet, const point &g);
  sf::Packet& operator >>(sf::Packet& packet, point &g);

  // player
  sf::Packet& operator <<(sf::Packet& packet, const player &g);
  sf::Packet& operator >>(sf::Packet& packet, player &g);
};
#endif
