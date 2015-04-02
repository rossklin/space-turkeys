#ifndef _STK_COM
#define _STK_COM

#include <string>
#include <vector>

#include <SFML/Network.hpp>

#include "types.h"

namespace st3{
  struct socket_t{
    sint id;
    sf::Socket::Status status;
    sf::TcpSocket *socket;
    sf::Packet data;

    socket_t();
    socket_t(sf::TcpSocket *s);
    bool send(sf::Packet &packet);
    bool receive();
    bool receive(sf::Packet &packet);
  };

  namespace server{
    struct client_t : public socket_t{
      void send_invalid();
      bool receive_query(protocol_t query);
    };

    struct com{
      std::vector<client_t> clients;

      com(std::vector<sf::TcpSocket*> c);
      void check_protocol(protocol_t query, sf::Packet &packet);
      void distribute_frames(std::vector<sf::Packet> &g, int &frame_count);
    };
  };
};

#endif
