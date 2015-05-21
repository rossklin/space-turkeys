#ifndef _STK_SOCKETT
#define _STK_SOCKETT

#include <SFML/Network.hpp>
#include "types.h"

namespace st3{
  /*! struct to handle data and status for sfml's tcp socket */
  struct socket_t{
    /*! client id */
    sint id;

    /*! latest status */
    sf::Socket::Status status;

    /*! the socket */
    sf::TcpSocket *socket;

    /*! packet to store latest received data */
    sf::Packet *data;

    /*! default constructor */
    socket_t();

    /*! construct with given socket */
    socket_t(sf::TcpSocket *s);

    /*! allocate the packet for data */
    void allocate_packet();

    /*! deallocate the packet */
    void deallocate_packet();

    /*! send a given packet
     @param packet the packet
     @return whether the send operation was successfull
    */
    bool send(sf::Packet &packet);

    /*! receive a packet
      @return whether the receive operation was successfull
    */
    bool receive();

    /*! receive a given packet
      @param packet the packet
      @return whether the receive operation was successfull 
    */
    bool receive(sf::Packet &packet);
  };
};
#endif
