#include <iostream>
#include "socket_t.h"

using namespace std;
using namespace st3;

network_error::network_error(string v) : runtime_error(v) {}

st3::socket_t::socket_t(){
  thread_com = NULL;
  id = -1;
}

bool socket_t::check_com() {
  return (!thread_com) || *thread_com == tc_run || *thread_com == tc_init;
}

bool st3::socket_t::send_packet(sf::Packet packet){
  setBlocking(false);
  while (check_com()) {
    switch(status = send(packet)){
    case sf::Socket::Disconnected:
      throw network_error("socket_t::send: disconnected.");
    case sf::Socket::Error:
      throw network_error("socket_t::send: error sending!");
    case sf::Socket::Done:
      cout << "socket_t::send_packet: done" << endl;
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
  data.clear();
  setBlocking(false);
  while (check_com()) {
    switch(status = receive(data)){
    case sf::Socket::Disconnected:
      throw network_error("socket_t::receive: disconnected.");
    case sf::Socket::Error:
      throw network_error("socket_t::receive: error receiving!");
    case sf::Socket::Done:
      cout << "socket_t::receive_packet: done" << endl;
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
