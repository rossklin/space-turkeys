#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <string>
#include <vector>

#include "data_frame.h"
#include "selector.h"
#include "socket_t.h"
#include "types.h"

namespace st3 {

/*! client side specifics */
namespace client {

struct cl_socket_t : public socket_t {
  int *instruction;
  bool check_com() override;
};

void query(cl_socket_t *socket, sf::Packet &pq, int &tc_in, int &tc_out);
void load_frames(cl_socket_t *socket, std::vector<client_game_data> &g, int &loaded, int &tc_in, int &tc_out);
void deserialize(client_game_data &f, sf::Packet &p, sint id);
};  // namespace client
};  // namespace st3
