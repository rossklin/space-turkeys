#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <thread>
#include <ctime>

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

#include "types.h"
#include "com.h"
#include "game_data.h"
#include "graphics.h"
#include "protocol.h"
#include "serialization.h"

using namespace std;
using namespace st3;

// blocking function: send package pq to socket, receive package pr
// from socket, then set the value of done to true (for threading)
void query(socket_t *socket, 
	   sf::Packet &pq,
	   sf::Packet &pr,
	   bool &done){
  protocol_t message;

  cout << "query: starting to send..." << endl;
  while (!socket -> send(pq)) sf::sleep(sf::milliseconds(100));
  cout << "query: starting to receive..." << endl;
  while (!socket -> receive(pr)) sf::sleep(sf::milliseconds(100));

  if (pr >> message){
    if (message == protocol::confirm){
      done = true;
      cout << "query: confirmed" << endl;
    }else if (message == protocol::invalid){
      cout << "query: server says invalid" << endl;
      exit(-1);
    }else if (message == protocol::standby){
      cout << "query: server says standby" << endl;
      exit(-1);
    }else if (message == protocol::complete){
      cout << "query: server says complete" << endl;
      exit(-1);
    }else {
      cout << "query: unknown response: " << message << endl;
      exit(-1);
    }
  }else{
    cout << "query: failed to unpack message" << endl;
    exit(-1);
  }
}

bool pre_step(socket_t &socket, window_t &window, game_data &g){
  bool done = false;
  sf::Packet packet, pq;
  sf::Font font;
  sf::Text text;

  // setup load text
  if (!font.loadFromFile(st3::font_dir + "arial.ttf")){
    cout << "error loading font" << endl;
    exit(-1);
  }

  text.setFont(font); 
  text.setString("(loading game data...)");
  text.setCharacterSize(24);
  text.setColor(sf::Color(200,200,200));
  // text.setStyle(sf::Text::Underlined);
  sf::Vector2f dims = window.getView().getSize();
  sf::FloatRect rect = text.getLocalBounds();
  text.setPosition(dims.x/2 - rect.width/2, dims.y/2 - rect.height/2);

  pq << protocol::game_round;

  cout << "pre_step: start: game data has " << g.ships.size() << " ships" << endl;
  
  // todo: handle response: complete
  thread t(query, 
	   &socket, 
	   ref(pq),
	   ref(packet), 
	   ref(done));

  while (!done){
    done |= !window.isOpen();

    sf::Event event;
    while (window.pollEvent(event)){
      if (event.type == sf::Event::Closed) window.close();
    }

    window.clear();
    draw_universe(window, g);
    window.draw(text);
    window.display();

    cout << "pre_step: loading data..." << endl;
    sf::sleep(sf::milliseconds(100));
  }

  cout << "pre_step: waiting for com thread to finish..." << endl;
  t.join();

  if (!(packet >> g)){
    cout << "pre_step: failed to deserialize game_data" << endl;
    exit(-1);
  }

  cout << "pre_step: end: game data has " << g.ships.size() << " ships" << endl;

  window.setView(sf::View(sf::FloatRect(0, 0, g.settings.width, g.settings.height)));

  return true;
}

void choice_step(socket_t &socket, window_t &window, game_data g){
  bool done = false;
  choice c;
  sf::Packet pq, pr;
  int count = 0;
  sf::Font font;
  sf::Text text;

  // setup load text
  if (!font.loadFromFile(st3::font_dir + "arial.ttf")){
    cout << "error loading font" << endl;
    exit(-1);
  }

  text.setFont(font); 
  text.setString("(sending choice to server...)");
  text.setCharacterSize(24);
  text.setColor(sf::Color(200,200,200));
  // text.setStyle(sf::Text::Underlined);
  sf::Vector2f dims = window.getView().getSize();
  sf::FloatRect rect = text.getLocalBounds();
  text.setPosition(dims.x/2 - rect.width/2, dims.y/2 - rect.height/2);

  // CREATE THE CHOICE (USER INTERFACE)

  cout << "choice step: start: game data has " << g.ships.size() << " ships" << endl;

  while (count++ < 40){
    sf::Event event;
    while (window.pollEvent(event)){
      if (event.type == sf::Event::Closed) exit(-1);

      // update choice with event and done
    }

    window.clear();
    draw_universe(window, g, c);
    window.display();

    sf::sleep(sf::milliseconds(100));
  }

  // SEND THE CHOICE TO SERVER
  
  done = false;
  pq << protocol::choice;
  pq << c;

  thread t(query, 
	   &socket, 
	   ref(pq),
	   ref(pr), 
	   ref(done));

  cout << "choice step: sending" << endl;

  while (!done){
    done |= !window.isOpen();

    sf::Event event;
    while (window.pollEvent(event)){
      if (event.type == sf::Event::Closed) window.close();
    }

    window.clear();
    draw_universe(window, g);
    window.draw(text);
    window.display();

    sf::sleep(sf::milliseconds(100));
  }

  cout << "choice step: waiting for query thread" << endl;
  t.join();
  cout << "choice step: complete" << endl;
}

void load_frames(socket_t *socket, vector<game_data> &g, int &loaded){
  sf::Packet pq, pr;
  bool done;

  // shifted indexing since g[0] is already loaded, so frame 0 goes in g[1]
  sint i = 0;
  while (i < g.size() - 1){
    pq.clear();
    pq << protocol::frame << i;
    query(socket, pq, pr, done);
    if (pr >> g[i + 1]){
      i++;
      loaded = i + 1;
    }else{
      sf::sleep(sf::milliseconds(10));
    }
  }
}

void simulation_step(socket_t &socket, window_t &window, game_data &g0){
  bool done = false;
  bool playing = true;
  int idx = 0;
  int loaded = 1;
  vector<game_data> g(g0.settings.frames_per_round + 1);
  g[0] = g0;

  thread t(load_frames, &socket, ref(g), ref(loaded));
  cout << "simulation start: game data has " << g0.ships.size() << " ships" << endl;

  while (!done){
    done |= !window.isOpen();
    done |= idx == g0.settings.frames_per_round;

    cout << "simulation loop: frame " << idx << " of " << loaded << " loaded frames, play = " << playing << endl;

    sf::Event event;
    while (window.pollEvent(event)){
      switch (event.type){
      case sf::Event::Closed:
	window.close();
	break;
      case sf::Event::KeyPressed:
	if (event.key.code == sf::Keyboard::Space){
	  playing = !playing;
	}
	break;
      }
    }

    window.clear();
    draw_universe(window, g[idx]);
    window.display();

    playing &= idx < loaded - 1;

    if (playing){
      idx++;
    }

    sf::sleep(sf::milliseconds(100));    
  }

  g0 = g.back();

  t.join();
}

void game_handler(socket_t &socket, window_t &window){
  game_data g;

  while (pre_step(socket, window, g)){
    choice_step(socket, window, g);
    simulation_step(socket, window, g);
  }
}

int main(){
  sf::TcpSocket socket;
  int width = 800;
  int height = 600;

  srand(time(NULL));

  // connect
  cout << "connecting...";
  if (socket.connect("127.0.0.1", 53000) != sf::Socket::Done){
    cout << "client: connection failed." << endl;
    return -1;
  }
  socket.setBlocking(false);
  cout << "done." << endl;

  // create graphics
  socket_t s(&socket);
  sf::Packet pq, pr;
  bool done;
  string mes = "Name_blabla";
  mes[rand()%mes.length()] = rand() % 256;
  mes[rand()%mes.length()] = rand() % 256;
  mes[rand()%mes.length()] = rand() % 256;

  cout << "sending name...";
  pq << protocol::connect << mes;

  query(&s, pq, pr, done);

  if (!(pr >> s.id)){
    cout << "server failed to provide id" << endl;
    exit(-1);
  }

  window_t window(sf::VideoMode(width, height), "SFML Turkeys!");
  game_handler(s, window);

  socket.disconnect();

  return 0;
}
