#pragma once

#include <SFML/Network.hpp>

#include "types.hpp"

namespace st3 {

/*! struct to handle data and status for sfml's tcp socket */
struct socket_t : public sf::TcpSocket {
  static const int tc_none = 0;
  static const int tc_init = 1;
  static const int tc_run = 2;
  static const int tc_complete = 4;
  static const int tc_stop = 8;
  static const int tc_failed = 16;
  static const int tc_game_complete = 32;
  static const int tc_ready_game = 64;
  static const int tc_bad_result = tc_stop | tc_failed;

  static const int default_timeout = 1000;

  /*! client id */
  static int idc;
  sint id;

  /*! latest status */
  sf::Socket::Status status;

  /*! packet to store latest received data */
  sf::Packet data;

  /*! default constructor */
  socket_t();

  virtual bool status_is_running() = 0;

  /*! send a given packet
     @param packet the packet
     @return whether the send operation was successfull
    */
  bool send_packet(sf::Packet packet, int timeout = default_timeout);

  /*! receive a packet
      @return whether the receive operation was successfull
    */
  bool receive_packet(int timeout = default_timeout);
};
};  // namespace st3
