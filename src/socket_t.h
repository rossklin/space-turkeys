#ifndef _STK_SOCKETT
#define _STK_SOCKETT

#include <SFML/Network.hpp>
#include "types.h"

namespace st3{
  /*! struct to handle data and status for sfml's tcp socket */
  struct socket_t : public sf::TcpSocket{
    /*! client id */
    sint id;

    /*! latest status */
    sf::Socket::Status status;

    /*! packet to store latest received data */
    sf::Packet data;

    /*! default constructor */
    socket_t();

    /*! send a given packet
     @param packet the packet
     @return whether the send operation was successfull
    */
    bool send_packet(sf::Packet &packet);

    /*! receive a packet
      @return whether the receive operation was successfull
    */
    bool receive_packet();
  };
};
#endif
