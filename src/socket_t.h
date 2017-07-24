#ifndef _STK_SOCKETT
#define _STK_SOCKETT

#include <stdexcept>
#include <SFML/Network.hpp>
#include "types.h"

namespace st3{
  class network_error : public std::runtime_error {
  public:
    network_error(std::string v);
  };
  
  /*! struct to handle data and status for sfml's tcp socket */
  struct socket_t : public sf::TcpSocket{
    static const int tc_init = 0;
    static const int tc_run = 1;
    static const int tc_stop = 2;
    static const int tc_complete = 4;
    static const int tc_failed = 8;
    static const int tc_game_complete = 16;
    
    /*! client id */
    sint id;
    int *thread_com;

    /*! latest status */
    sf::Socket::Status status;

    /*! packet to store latest received data */
    sf::Packet data;

    /*! default constructor */
    socket_t();

    bool check_com();

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
