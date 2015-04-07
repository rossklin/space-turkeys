#ifndef _STK_COMSERVER
#define _STK_COMSERVER

#include <string>
#include <vector>

#include <SFML/Network.hpp>

#include "types.h"
#include "socket_t.h"

namespace st3{
  namespace server{
    struct client_t : public socket_t{
      std::string name;

      void send_invalid();
      bool receive_query(protocol_t query);
    };

    struct com{
      std::vector<client_t> clients;

      com(std::vector<sf::TcpSocket*> c);
      void check_protocol(protocol_t query, sf::Packet &packet);
      void check_protocol(protocol_t query, std::vector<sf::Packet> &packets);
      void distribute_frames(std::vector<sf::Packet> &g, int &frame_count);
    };
  };
};

#endif