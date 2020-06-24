#include "socket_t.h"

#include <chrono>
#include <iostream>

using namespace std;
using namespace st3;

int socket_t::idc = 0;
st3::socket_t::socket_t() {
  id = idc++;
}

bool st3::socket_t::send_packet(sf::Packet packet, int timeout) {
  chrono::time_point<chrono::system_clock> start = chrono::system_clock::now();
  std::chrono::duration<double> elapsed;

  setBlocking(false);
  do {
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
    elapsed = (chrono::system_clock::now() - start);

    if (timeout > 0) {
      sf::sleep(sf::milliseconds(10));
    }
  } while (check_com() && elapsed.count() < timeout);

  if (timeout > 0 && elapsed.count() > timeout) {
    throw network_error("socket_t::send: timeout");
  }

  // return false when aborted by thread_com
  return false;
}

bool st3::socket_t::receive_packet(int timeout) {
  chrono::time_point<chrono::system_clock> start = chrono::system_clock::now();
  std::chrono::duration<double> elapsed;

  data.clear();
  setBlocking(false);
  do {
    std::chrono::duration<double> elapsed = (chrono::system_clock::now() - start);

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
    elapsed = (chrono::system_clock::now() - start);

    if (timeout > 0) {
      sf::sleep(sf::milliseconds(10));
    }
  } while (check_com() && elapsed.count() < timeout);

  if (timeout > 0 && elapsed.count() > timeout) {
    throw network_error("socket_t::send: timeout");
  }

  // return false when aborted by thread_com
  return false;
}
