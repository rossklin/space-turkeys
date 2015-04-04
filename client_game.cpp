#include <iostream>
#include <thread>

#include "client_game.h"
#include "graphics.h"
#include "com_client.h"
#include "protocol.h"
#include "serialization.h"

using namespace std;
using namespace st3;
using namespace st3::client;

sf::FloatRect fixrect(sf::FloatRect r);

void st3::client::game::run(){
  // game loop
  while (pre_step()){
    choice_step();
    simulation_step();
  }
}

// ****************************************
// PRE_STEP
// ****************************************

bool st3::client::game::pre_step(){
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
  point dims = window.getView().getSize();
  sf::FloatRect rect = text.getLocalBounds();
  text.setPosition(dims.x/2 - rect.width/2, dims.y/2 - rect.height/2);

  pq << protocol::game_round;

  cout << "pre_step: start: game data has " << data.ships.size() << " ships" << endl;
  
  // todo: handle response: complete
  thread t(query, 
	   socket, 
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
    draw_universe(window, data);
    window.draw(text);
    window.display();

    cout << "pre_step: loading data..." << endl;
    sf::sleep(sf::milliseconds(100));
  }

  cout << "pre_step: waiting for com thread to finish..." << endl;
  t.join();

  if (!(packet >> data)){
    cout << "pre_step: failed to deserialize game_data" << endl;
    exit(-1);
  }

  cout << "pre_step: end: game data has " << data.ships.size() << " ships" << endl;

  window.setView(sf::View(sf::FloatRect(0, 0, data.settings.width, data.settings.height)));

  return true;
}

// ****************************************
// CHOICE STEP
// ****************************************

sf::FloatRect fixrect(sf::FloatRect r){
  // fix reverse selection
  if (r.width < 0){
    r.left += r.width;
    r.width *= -1;
  }

  if (r.height < 0){
    r.top += r.height;
    r.height *= -1;
  }

  return r;
}

void st3::client::game::area_select(sf::FloatRect r){
  sf::FloatRect rect = fixrect(r);

  selected_solars.clear();
  
  for (auto i = data.solars.begin(); i != data.solars.end(); i++){
    if (rect.contains(i -> second.position)){
      selected_solars.insert(i -> first);
    }
  }
}

void st3::client::game::click_at(point p){
  idtype id = -1;
  bool addsel = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift);
  bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LCtrl);

  // find id of solar at p
  for (auto i = data.solars.begin(); i != data.solars.end() && id < 0; i++){
    if (l2norm(p - i -> second.position) < i -> second.radius){
      id = i -> first;
    }
  }

  if (ctrl){
  }else{
    if (addsel){
      // add to selection
      if (id > -1) selected_solars.insert(id);
    }else{
      // select/target
      // todo: write me
    }
  }
}

void st3::client::game::choice_event(sf::Event e){
  static sf::FloatRect srect;

  switch (e.type){
  case sf::Event::MouseButtonPressed:
    if (e.mouseButton.button == sf::Mouse::Left){
      point p = window.mapPixelToCoords(sf::Vector2i(e.mouseButton.x, e.mouseButton.y));
      srect.left = p.x;
      srect.top = p.y;
    }
  case sf::Event::MouseButtonReleased:
    if (e.mouseButton.button == sf::Mouse::Left){
      if (srect.width || srect.height){
	area_select(srect);
      }else{
	click_at(window.mapPixelToCoords(sf::Vector2i(e.mouseButton.x, e.mouseButton.y)));
      }
    }
  };
}

void st3::client::game::choice_step(){
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
  point dims = window.getView().getSize();
  sf::FloatRect rect = text.getLocalBounds();
  text.setPosition(dims.x/2 - rect.width/2, dims.y/2 - rect.height/2);

  // CREATE THE CHOICE (USER INTERFACE)

  cout << "choice step: start: game data has " << data.ships.size() << " ships" << endl;

  while (count++ < 40){
    sf::Event event;
    while (window.pollEvent(event)){
      if (event.type == sf::Event::Closed) exit(-1);
      choice_event(event);
    }

    window.clear();
    draw_universe(window, data, genchoice);
    window.display();

    sf::sleep(sf::milliseconds(100));
  }

  // SEND THE CHOICE TO SERVER
  
  done = false;
  pq << protocol::choice;
  pq << c;

  thread t(query, 
	   socket, 
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
    draw_universe(window, data);
    window.draw(text);
    window.display();

    sf::sleep(sf::milliseconds(100));
  }

  cout << "choice step: waiting for query thread" << endl;
  t.join();
  cout << "choice step: complete" << endl;
}

// ****************************************
// SIMULATION STEP
// ****************************************

void st3::client::game::simulation_step(){
  bool done = false;
  bool playing = true;
  int idx = 0;
  int loaded = 1;
  vector<game_data> g(data.settings.frames_per_round + 1);
  g[0] = data;

  thread t(load_frames, socket, ref(g), ref(loaded));
  cout << "simulation start: game data has " << data.ships.size() << " ships" << endl;

  while (!done){
    done |= !window.isOpen();
    done |= idx == data.settings.frames_per_round;

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

  data = g.back();

  t.join();
}

// ****************************************
// GRAPHICS
// ****************************************

void st3::client::game::draw_universe(){
  sf::Color col;
  sf::Vertex svert[4];

  // draw ships
  for (auto x : data.ships) {
    ship s = x.second;
    if (!s.was_killed){
      col = sfcolor(data.players[s.owner].color);
      svert[0] = sf::Vertex(point(10, 0), col);
      svert[1] = sf::Vertex(point(-10, -5), col);
      svert[2] = sf::Vertex(point(-10, 5), col);
      svert[3] = sf::Vertex(point(10, 0), col);
      
      sf::Transform t;
      t.translate(s.position.x, s.position.y);
      t.rotate(s.angle / (2 * M_PI) * 360);
      window.draw(svert, 4, sf::LinesStrip, t);
    }
  }

  // draw solars
  for (auto x : data.solars){
    solar s = x.second;
    sf::CircleShape sol(s.radius);
    sol.setFillColor(sf::Color(10,20,30,40));
    sol.setOutlineThickness(s.radius / 3);
    col = s.owner > -1 ? sfcolor(data.players[s.owner].color) : sf::Color(150,150,150);
    sol.setOutlineColor(col);
    sol.setPosition(s.position.x, s.position.y);
    window.draw(sol);
  }
}
