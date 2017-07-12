#include <iostream>
#include "socket_t.h"

using namespace std;
using namespace st3;

st3::socket_t::socket_t(){
  id = -1;
}

bool st3::socket_t::send_packet(sf::Packet &packet){
  switch(status = send(packet)){
  case sf::Socket::Disconnected:
    throw runtime_error("socket_t::send: disconnected.");
  case sf::Socket::Error:
    throw runtime_error("socket_t::send: error sending!");
  case sf::Socket::Done:
    // cout << "socket_t::send: done" << endl;
    return true;
  case sf::Socket::NotReady:
    // cout << "socket_t::send: not ready" << endl;
    return false;
  case sf::Socket::Partial:
    throw runtime_error("socket_t::send: status partial in blocking mode!");
  default:
    throw runtime_error("socket_t::send: unknown status: " + to_string(status));
  }
}

bool st3::socket_t::receive_packet(){
  data.clear();
  switch(status = receive(data)){
  case sf::Socket::Disconnected:
    throw runtime_error("socket_t::receive: disconnected.");
  case sf::Socket::Error:
    throw runtime_error("socket_t::receive: error sending!");
  case sf::Socket::Done:
    // cout << "socket_t::receive: done" << endl;
    return true;
  case sf::Socket::NotReady:
    // cout << "socket_t::receive: not ready" << endl;
    return false;
  case sf::Socket::Partial:
    throw runtime_error("socket_t::receive: status partial in blocking mode!");
  default:
    throw runtime_error("socket_t::receive: unknown status: " + to_string(status));
  }
}
