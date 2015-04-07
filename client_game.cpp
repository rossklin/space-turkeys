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
bool add2selection();
bool ctrlsel();

void st3::client::game::command_points(command c, point &from, point &to){
 switch (c.source.type){
  case source_t::SOLAR:
    from = data.solars[c.source.id].position;
    break;
  case source_t::FLEET:
    from = data.fleets[c.source.id].position;
    break;
  default:
    cout << "invalid source type: " << c.source.type << endl;
    exit(-1);
  }

 switch (c.target.type){
  case target_t::SOLAR:
    to = data.solars[c.target.id].position;
    break;
  case target_t::FLEET:
    to = data.fleets[c.target.id].position;
    break;
 case target_t::POINT:
   to = c.target.position;
   break;
 default:
   cout << "invalid target type: " << c.target.type << endl;
   exit(-1);
 }

 return;
}

void st3::client::game::add_command(command c){
  point from, to;
  if (command_exists(c)) return;
  idtype id = comid++;
  command_points(c, from, to);
  command_selectors[id] = new command_selector(c, from, to);
  entity_commands[c.source].insert(id);
}

void st3::client::game::remove_command(idtype id){
  if (command_selectors.count(id)){
    entity_commands[command_selectors[id] -> c.source].erase(id);
    delete command_selectors[id];
    command_selectors.erase(id);
  }
}

void st3::client::game::clear_selectors(){
  comid = 0;

  for (auto x : entity_selectors){
    delete x.second;
  }

  for (auto x : command_selectors){
    delete x.second;
  }

  entity_selectors.clear();
  command_selectors.clear();
  entity_commands.clear();
}

void st3::client::game::initialize_selectors(){
  source_t s;
  clear_selectors();

  for (auto x : data.solars){
    s.id = x.first;
    s.type = source_t::SOLAR;
    entity_selectors[s] = new solar_selector(&x.second, s.id, x.second.owner == socket.id);
  }

  for (auto x : data.fleets){
    s.id = x.first;
    s.type = source_t::FLEET;
    entity_selectors[s] = new fleet_selector(&x.second, s.id, x.second.owner == socket.id);
  }
}

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
    draw_universe(data);
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

void st3::client::game::area_select(){
  sf::FloatRect rect = fixrect(srect);

  if (!add2selection()) deselect_all();
  
  for (auto x : entity_selectors){
    x.second -> selected = x.second -> owned && x.second -> inside_rect(rect);
  }
}

void st3::client::game::command2point(point p){
  command c;
  c.target.id = -1;
  c.target.type = target_t::POINT;
  c.target.position = p;

  for (auto x : entity_selectors){
    if (x.second -> selected){
      c.source = x.second -> command_source();
      add_command(c);
    }
  }
}

void st3::client::game::command2entity(entity_selector *s){
  command c;
  source_t x = s -> command_source();
  c.target.id = x.id;
  c.target.type = x.type;

  for (auto x : entity_selectors){
    if (x.second != s && x.second -> selected){
      c.source = x.second -> command_source();
      add_command(c);
    }
  }
}

void st3::client::game::click_at(point p, sf::Mouse::Button button){
  float max_click_dist = 20;
  float dmin = max_click_dist;
  bool found = false;
  entity_selector *esel;
  command_selector *csel;
  float d;
  bool left = button == sf::Mouse::Left;
  bool right = button == sf::Mouse::Right;

  // consider merging solar and fleet selection to a common interface?
  // find closest entity to p
  for (auto x : entity_selectors){
    if (x.second -> contains_point(p, d) && d < dmin){
      dmin = d;
      esel = x.second;
    }
  }

  // find closest command to p
  for (auto x : command_selectors){
    if (x.second -> contains_point(p, d) && d < dmin){
      dmin = d;
      csel = x.second;
    }
  }

  if (left){
    if (!add2selection()) deselect_all();
    if (csel){
      csel -> selected = !(csel -> selected);
    }else if (esel && esel -> owned){
      esel -> selected = !(esel -> selected);
    }else if (esel){
      command2entity(esel);
    }
  }else if (right){
    if (esel){
      command2entity(esel);
    }else{
      command2point(p);
    }
    if (!add2selection()) deselect_all();
  }
}

bool st3::client::game::choice_event(sf::Event e){

  switch (e.type){
  case sf::Event::MouseButtonPressed:
    if (e.mouseButton.button == sf::Mouse::Left){
      point p = window.mapPixelToCoords(sf::Vector2i(e.mouseButton.x, e.mouseButton.y));
      srect.left = p.x;
      srect.top = p.y;
    }
    break;
  case sf::Event::MouseButtonReleased:
    if (e.mouseButton.button == sf::Mouse::Left){
      if (srect.width || srect.height){
	area_select();
      }
    }
    click_at(window.mapPixelToCoords(sf::Vector2i(e.mouseButton.x, e.mouseButton.y)), e.mouseButton.button);
  };
}

void st3::client::game::choice_step(){
  bool done = false;
  sf::Packet pq, pr;
  int count = 0;
  sf::Font font;
  sf::Text text;

  initialize_selectors();

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

  while (!done){
    sf::Event event;
    while (window.pollEvent(event)){
      if (event.type == sf::Event::Closed) exit(-1);
      done |= choice_event(event);
    }

    window.clear();
    draw_universe(data);
    window.display();

    sf::sleep(sf::milliseconds(100));
  }

  // SEND THE CHOICE TO SERVER
  
  choice c = build_choice();
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
    draw_universe(data);
    window.draw(text);
    window.display();

    sf::sleep(sf::milliseconds(100));
  }

  clear_selectors();

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
    draw_universe(g[idx]);
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

void st3::client::game::draw_universe(game_data &g){
  sf::Color col;
  sf::Vertex svert[4];

  // draw ships
  for (auto x : g.ships) {
    ship s = x.second;
    if (!s.was_killed){
      col = sfcolor(g.players[s.owner].color);
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
  for (auto x : g.solars){
    solar s = x.second;
    sf::CircleShape sol(s.radius);
    sol.setFillColor(sf::Color(10,20,30,40));
    sol.setOutlineThickness(s.radius / 3);
    col = s.owner > -1 ? sfcolor(g.players[s.owner].color) : sf::Color(150,150,150);
    sol.setOutlineColor(col);
    sol.setPosition(s.position.x, s.position.y);
    window.draw(sol);
  }
}


void st3::client::game::draw_universe_ui(){
  sf::Color col;
  sf::Vertex svert[4];

  // todo: check selected

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

  // draw commands
}

// ****************************************
// UTILITY FUNCTIONS
// ****************************************

bool add2selection(){
  sf::Keyboard::isKeyPressed(sf::Keyboard::LShift);
}

bool ctrlsel(){
  sf::Keyboard::isKeyPressed(sf::Keyboard::LControl);
}

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
