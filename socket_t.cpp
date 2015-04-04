#include <iostream>
#include "socket_t.h"

using namespace std;
using namespace st3;

st3::socket_t::socket_t(){
  socket = 0;
  id = -1;
}

st3::socket_t::socket_t(sf::TcpSocket *s){
  socket = s;
  id = -1;
}

void st3::socket_t::allocate(){
  data = new sf::Packet();
}

void st3::socket_t::deallocate(){
  delete data;
}

bool st3::socket_t::send(sf::Packet &packet){
  switch(status = socket -> send(packet)){
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

bool st3::socket_t::receive(){
  receive(*data);
}

bool st3::socket_t::receive(sf::Packet &packet){
  switch(status = socket -> receive(packet)){
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
