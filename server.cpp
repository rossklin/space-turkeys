#include <iostream>

#include "game_handler.h"
#include "com.h"
#include "protocol.h"

using namespace std;
using namespace st3;
using namespace st3::server;

int main(){
  sf::TcpListener listener;

  cout << "binding listener ...";
  // bind the listener to a port
  if (listener.listen(53000) != sf::Socket::Done) {
    cout << "failed." << endl;
    return -1;
  }

  cout << "done." << endl;

  vector<sf::TcpSocket*> clients;

  // accept a new connection
  while (clients.size() < 2){
    sf::TcpSocket *c = new sf::TcpSocket();
    cout << "listening...";
    if (listener.accept(*c) != sf::Socket::Done) {
      cout << "error." << endl;
      return -1;
    }
    c -> setBlocking(false);
    cout << "client connected!" << endl;
    clients.push_back(c);
  }

  listener.close();

  com c(clients);
  game_data g;

  // formalities
  vector<sf::Packet> packets(clients.size());
  for (sint i = 0; i < c.clients.size(); i++){
    packets[i] << protocol::confirm << i;
    c.clients[i].id = i;
  }

  c.check_protocol(protocol::connect, packets);

  for (auto x = c.clients.begin(); x != c.clients.end(); x++){
    if (!(x -> data >> x -> name)){
      cout << "client " << x -> id << " failed to provide name." << endl;
      exit(-1);
    }
  }
  
  g.build();

  game_handler(c, g);

  for (auto x : clients) {
    x -> disconnect();
    delete x;
  }

  return 0;
}
