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

void st3::client::game::add_command(command c, point from, point to){
  if (command_exists(c)) return;
  idtype id = comid++;
  source_t key = identifier::make(identifier::command, id);
  entity_selectors[key] = new command_selector(id, from, to);
  entity_commands[c.source].insert(key);
  commands[id] = c;
}

// remove command selector and associated command
void st3::client::game::remove_command(source_t key){
  if (identifier::get_type(key).compare(identifier::command)){
    cout << "remove command: is not a command: " << key << endl;
    exit(-1);
  }

  if (entity_selectors.count(key)){
    command_selector *s = (command_selector*)entity_selectors[key];
    source_t source_key = commands[s -> id].source;

    // remove this command from it's parent's list
    entity_commands[source_key].erase(key);

    // remove this command's list of child commands
    entity_commands.erase(key);

    // remove this command's selector
    entity_selectors.erase(key);
    delete s;
  }
}

void st3::client::game::clear_selectors(){
  comid = 0;

  for (auto x : entity_selectors){
    delete x.second;
  }

  entity_selectors.clear();
  entity_commands.clear();
  commands.clear();
}

void st3::client::game::initialize_selectors(){
  source_t s;
  clear_selectors();

  for (auto x : data.solars){
    s = identifier::make(identifier::solar, x.first);
    entity_selectors[s] = new solar_selector(x.first, x.second.owner == socket.id, x.second.position, x.second.radius);
  }

  for (auto x : data.fleets){
    s = identifier::make(identifier::fleet, x.first);
    entity_selectors[s] = new solar_selector(x.first, x.second.owner == socket.id, x.second.position, x.second.radius);
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
    x.second -> selected = x.second -> owned && x.second -> area_selectable && x.second -> inside_rect(rect);
  }
}

void st3::client::game::command2point(point p){
  command c;
  point from, to;
  c.target = identifier::make(identifier::point, p);
  to = p;

  for (auto x : entity_selectors){
    if (x.second -> selected){
      c.source = x.second -> command_source();
      from = x.second -> position;
      add_command(c, from, to);
    }
  }
}

void st3::client::game::command2entity(source_t key){
  command c;
  point from, to;
  entity_selector *s = entity_selectors[key];
  c.target = s -> command_source();
  to = s -> position;

  for (auto x : entity_selectors){
    if (x.second != s && x.second -> selected){
      c.source = x.second -> command_source();
      from = x.second -> position;
      add_command(c, from, to);
    }
  }
}

source_t st3::client::game::entity_at(point p){
  float max_click_dist = 20;
  float dmin = max_click_dist;
  float d;
  source_t key = "";

  // find closest entity to p
  for (auto x : entity_selectors){
    if (x.second -> contains_point(p, d) && d < dmin){
      dmin = d;
      key = x.first;
    }
  }

  return key;
}

void st3::client::game::target_at(point p){
  source_t key = entity_at(p);
  auto it = entity_selectors.find(key);

  if (it != entity_selectors.end()){
    command2entity(key);
  }else{
    command2point(p);
  }
}

void st3::client::game::select_at(point p){
  source_t key = entity_at(p);
  auto it = entity_selectors.find(key);
 
  if (!add2selection()) deselect_all();

  if (it != entity_selectors.end()){
    it -> second -> selected = !(it -> second -> selected);
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
    point p = window.mapPixelToCoords(sf::Vector2i(e.mouseButton.x, e.mouseButton.y));
    if (e.mouseButton.button == sf::Mouse::Left){
      if (srect.width || srect.height){
	area_select();
      } else {
	select_at(p);
      }
    }else if (e.mouseButton.button == sf::Mouse::Right){
      target_at(p);
    }
    break;
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
    draw_universe_ui();
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
