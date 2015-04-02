#include <string>

#include "game_data.h"

using namespace std;

void data_loader(sf::TcpSocket &s, game_data &g){
  sf::Packet p, pq;
  bool q = true;
  string message = "frame";
  clock_t t = clock();

  pq << message;

  while (g.run){
    if (q){
      // query
      switch(s.send(pq)){
      case sf::Socket::Disconnected:
	cout << "client: disconnected" << endl;
	exit(-1);
      case sf::Socket::Error:
	cout << "client: error while sending query" << endl;
	break;
      case sf::Socket::NotReady:
	break;
      case sf::Socket::Done:
	q = false;
      }
    }else{
      // load data
      switch(s.receive(p)){
      case sf::Socket::Disconnected:
        cout << "disconnected." << endl;
	exit(-1);
      case sf::Socket::Error:
        cout << "error receiving!" << endl;
        break;
      case sf::Socket::Done:
	q = true;

        // update data
        if (p >> g){
          double dt = (clock() - t) / (double)CLOCKS_PER_SEC;
          t = clock();
          cout << "update received, dt = " << dt << endl;
        }else{
	  cout << "error unpacking!" << endl;
	}
      }
    }

    sf::sleep(sf::milliseconds(10));
  }
}

void game(sf::TcpSocket &socket, sf::RenderWindow &window){
  game_data g;
  g.run = true;

  thread t(ref(socket), ref(g));

  while (g.run){
    paint_game(g);
    sf::sleep(sf::milliseconds(50));
  }  
}
