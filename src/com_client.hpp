#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <memory>
#include <string>
#include <vector>

#include "game_base_data.hpp"
#include "socket_t.hpp"
#include "types.hpp"

namespace st3 {

/*! client side specifics */

struct cl_socket_t : public socket_t {
  int instruction;
  bool check_com() override;
};

typedef std::shared_ptr<cl_socket_t> cl_socket_ptr;

namespace client {
bool query(cl_socket_ptr socket, sf::Packet &pq);
bool load_frames(cl_socket_ptr socket, std::vector<game_base_data> &g, int &loaded);
void deserialize(game_base_data &f, sf::Packet &p, sint id);
};  // namespace client
};  // namespace st3
