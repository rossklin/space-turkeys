#include <iostream>

#include "game_handler.h"
#include "com_server.h"
#include "protocol.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace st3::server;

int main(int argc, char **argv){
  sf::TcpListener listener;
  int num_clients = argc == 2 ? atoi(argv[1]) : 2;

  srand(time(NULL));

  cout << "binding listener ...";
  // bind the listener to a port
  if (listener.listen(53000) != sf::Socket::Done) {
    cout << "failed." << endl;
    return -1;
  }

  cout << "done." << endl;

  vector<sf::TcpSocket*> clients;

  // accept a new connection
  while (clients.size() < num_clients){
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

  cout << "starting with " << clients.size() << " clients" << endl;

  com c(clients);
  game_data g;

  // formalities
  vector<sf::Packet> packets;
  packets.resize(c.clients.size());

  for (sint i = 0; i < c.clients.size(); i++){
    packets[i] << protocol::confirm << c.clients[i].id;
    cout << "loading packet " << i << " with id " << c.clients[i].id << endl;
  }

  c.check_protocol(protocol::connect, packets);

  for (auto x = c.clients.begin(); x != c.clients.end(); x++){
    if (!(*x -> data >> x -> name)){
      cout << "client " << x -> id << " failed to provide name." << endl;
      exit(-1);
    }
  }

  vector<sint> colbuf = utility::different_colors(c.clients.size());
  int i = 0;
  for (auto x : c.clients){
    player p;
    p.name = x.name;
    p.color = colbuf[i++];
    g.players[x.id] = p;
  }

  // g.settings.frames_per_round = 10;
  
  g.build();

  game_handler(c, g);

  c.deallocate();
  for (auto x : clients) {
    x -> disconnect();
    delete x;
  }

  return 0;
}
