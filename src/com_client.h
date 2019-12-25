#ifndef _STK_COMCLIENT
#define _STK_COMCLIENT

#include <string>
#include <vector>

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

#include "data_frame.h"
#include "selector.h"
#include "socket_t.h"
#include "types.h"

namespace st3 {

/*! client side specifics */
namespace client {
void query(socket_t *socket, sf::Packet &pq, int &tc_in, int &tc_out);
void load_frames(socket_t *socket, std::vector<data_frame> &g, int &loaded, int &tc_in, int &tc_out);
void deserialize(data_frame &f, sf::Packet &p, sint id);
};  // namespace client
};  // namespace st3

#endif
