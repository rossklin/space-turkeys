#include <iostream>

#include "game_handler.h"
#include "com.h"

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

  cout << "listening...";
  // accept a new connection
  sf::TcpSocket client;
  if (listener.accept(client) != sf::Socket::Done) {
    cout << "error." << endl;
    return -1;
  }

  listener.close();

  client.setBlocking(false);
  cout << "client connected!" << endl;

  vector<sf::TcpSocket*> clients;
  clients.push_back(&client);

  com c(clients);
  game_data g;
  
  g.build();

  game_handler(c, g);

  return 0;
}
