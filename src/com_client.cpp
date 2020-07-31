#include "com_client.hpp"

#include <exception>
#include <iostream>
#include <mutex>
#include <vector>

#include "client_game.hpp"
#include "game_base_data.hpp"
#include "game_object.hpp"
#include "protocol.hpp"
#include "selector.hpp"
#include "serialization.hpp"
#include "utility.hpp"

using namespace std;
using namespace st3;
using namespace st3::client;

bool cl_socket_t::check_com() {
  return instruction == tc_run;
}

bool st3::client::query(cl_socket_ptr socket, sf::Packet &pq) {
  static mutex m;
  scoped_lock lock(m);
  protocol_t message;

  try {
    if (!socket->send_packet(pq)) {
      return false;
    }

    if (!socket->receive_packet()) {
      return false;
    }

    if (socket->data >> message) {
      if (message == protocol::confirm) {
        return true;
      } else if (message == protocol::standby) {
        // wait a little while then try again
        sf::sleep(sf::milliseconds(500));
        return query(socket, pq);
      } else if (message == protocol::invalid) {
        throw network_error("query: server says invalid");
      } else if (message == protocol::aborted) {
        cout << "query: server says game aborted" << endl;
        return false;
      } else {
        throw network_error("query: unknown response: " + message);
      }
    } else {
      throw network_error("query: failed to unpack message");
    }
  } catch (const exception &e) {
    if (auto p = game::gmain.lock()) {
      p->terminate_with_message("Communication error: " + string(e.what()));
    } else {
      cout << "Exception in client::query but game references is invalid! " << e.what() << endl;
    }
    return false;
  }
}

// unpack entity package and generate corresponding selector objects in data frame
void client::deserialize(game_base_data &f, sf::Packet &p, sint id) {
  game_base_data ep;

  if (f.entity.size()) {
    throw classified_error("client::deserialize: data frame contains entities!");
  }

  if (!(p >> ep)) {
    throw network_error("deserialize: package empty!");
  }

  f.settings = ep.settings;
  f.players = ep.players;
  f.remove_entities = ep.remove_entities;
  f.terrain = ep.terrain;

  for (auto x : ep.all_entities<game_object>()) {
    sf::Color col;

    if (x->owner >= 0) {
      col = sf::Color(f.players[x->owner].color);
    } else {
      col = sf::Color(150, 150, 150);
    }

    entity_selector::ptr obj;
    if (x->isa(ship::class_id)) {
      obj = ship_selector::create(*utility::guaranteed_cast<ship>(x), col, x->owner == id);
    } else if (x->isa(fleet::class_id)) {
      obj = fleet_selector::create(*utility::guaranteed_cast<fleet>(x), col, x->owner == id);
    } else if (x->isa(solar::class_id)) {
      obj = solar_selector::create(*utility::guaranteed_cast<solar>(x), col, x->owner == id);
    } else if (x->isa(waypoint::class_id)) {
      obj = waypoint_selector::create(*utility::guaranteed_cast<waypoint>(x), col, x->owner == id);
    } else {
      throw network_error("com_client::deserialize: Failed sanity check: class not recognized!");
    }

    f.add_entity(obj);
  }

  // deallocate temporary entity data
  ep.clear_entities();
}
