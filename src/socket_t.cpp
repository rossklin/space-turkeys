#include "socket_t.h"

#include <chrono>
#include <iostream>

using namespace std;
using namespace st3;

st3::socket_t::socket_t() {
  id = -1;
}

bool st3::socket_t::send_packet(sf::Packet packet) {
  chrono::time_point<chrono::system_clock> start = chrono::system_clock::now();

  setBlocking(false);
  while (check_com()) {
    std::chrono::duration<double> elapsed = (chrono::system_clock::now() - start);
    if (elapsed.count() > timeout) {
      throw network_error("socket_t::send: timeout");
    }

    switch (status = send(packet)) {
      case sf::Socket::Disconnected:
        throw network_error("socket_t::send: disconnected.");
      case sf::Socket::Error:
        throw network_error("socket_t::send: error sending!");
      case sf::Socket::Done:
        return true;
      case sf::Socket::NotReady:
        // continue
        break;
      case sf::Socket::Partial:
        // continue
        break;
      default:
        throw network_error("socket_t::send: unknown status: " + to_string(status));
    }

    sf::sleep(sf::milliseconds(10));
  }

  // return false when aborted by thread_com
  return false;
}

bool st3::socket_t::receive_packet() {
  chrono::time_point<chrono::system_clock> start = chrono::system_clock::now();

  data.clear();
  setBlocking(false);
  while (check_com()) {
    std::chrono::duration<double> elapsed = (chrono::system_clock::now() - start);
    if (elapsed.count() > timeout) {
      throw network_error("socket_t::receive: timeout");
    }

    switch (status = receive(data)) {
      case sf::Socket::Disconnected:
        throw network_error("socket_t::receive: disconnected.");
      case sf::Socket::Error:
        throw network_error("socket_t::receive: error receiving!");
      case sf::Socket::Done:
        return true;
      case sf::Socket::NotReady:
        break;
      case sf::Socket::Partial:
        break;
      default:
        throw network_error("socket_t::receive: unknown status: " + to_string(status));
    }

    sf::sleep(sf::milliseconds(10));
  }

  // return false when aborted by thread_com
  return false;
}
