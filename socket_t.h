#ifndef _STK_SOCKETT
#define _STK_SOCKETT

#include <SFML/Network.hpp>
#include "types.h"

namespace st3{
  struct socket_t{
    sint id;
    sf::Socket::Status status;
    sf::TcpSocket *socket;
    sf::Packet *data;

    socket_t();
    socket_t(sf::TcpSocket *s);
    void allocate();
    void deallocate();
    bool send(sf::Packet &packet);
    bool receive();
    bool receive(sf::Packet &packet);
  };
};
#endif
