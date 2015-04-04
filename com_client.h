#ifndef _STK_COMCLIENT
#define _STK_COMCLIENT

#include <string>
#include <vector>

#include <SFML/Network.hpp>

#include "types.h"
#include "socket_t.h"

namespace st3{
  struct game_data;

  namespace client{
    void query(socket_t socket, 
	       sf::Packet &pq,
	       sf::Packet &pr,
	       bool &done);
    void load_frames(socket_t socket, std::vector<game_data> &g, int &loaded);
  };
};

#endif
