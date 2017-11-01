#ifndef _STK_SOCKETT
#define _STK_SOCKETT

#include <SFML/Network.hpp>
#include "types.h"

namespace st3{
  
  /*! struct to handle data and status for sfml's tcp socket */
  struct socket_t : public sf::TcpSocket{
    static const int tc_none = 0;
    static const int tc_init = 1;
    static const int tc_run = 2;
    static const int tc_complete = 4;
    static const int tc_stop = 8;
    static const int tc_failed = 16;
    static const int tc_game_complete = 32;
    static const int tc_ok_result = tc_run | tc_complete;
    static const int tc_bad_result = ~tc_ok_result;
    static const int timeout = 1000;
    
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
    bool send_packet(sf::Packet packet);

    /*! receive a packet
      @return whether the receive operation was successfull
    */
    bool receive_packet();
  };
};
#endif
