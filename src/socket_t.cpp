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
    cout << "socket_t::send: disconnected." << endl;
    exit(-1);
  case sf::Socket::Error:
    cout << "socket_t::send: error sending!" << endl;
    exit(-1);
  case sf::Socket::Done:
    // cout << "socket_t::send: done" << endl;
    return true;
  case sf::Socket::NotReady:
    // cout << "socket_t::send: not ready" << endl;
    return false;
  default:
    cout << "socket_t::send: unknown status: " << status << endl;
    exit(-1);
  }
}

bool st3::socket_t::receive_packet(){
  data.clear();
  switch(status = receive(data)){
  case sf::Socket::Disconnected:
    cout << "socket_t::receive: disconnected." << endl;
    exit(-1);
  case sf::Socket::Error:
    cout << "socket_t::receive: error sending!" << endl;
    exit(-1);
  case sf::Socket::Done:
    // cout << "socket_t::receive: done" << endl;
    return true;
  case sf::Socket::NotReady:
    // cout << "socket_t::receive: not ready" << endl;
    return false;
  default:
    cout << "socket_t::send: unknown status: " << status << endl;
    exit(-1);
  }
}
