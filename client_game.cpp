#include <iostream>
#include <thread>

#include <SFML/Graphics.hpp>

#include "client_game.h"
#include "graphics.h"
#include "com_client.h"
#include "protocol.h"
#include "serialization.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace st3::client;

sf::FloatRect fixrect(sf::FloatRect r);
bool add2selection();
bool ctrlsel();

// ****************************************
// GAME STEPS
// ****************************************

void st3::client::game::run(){
  area_select_active = false;

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
  game_data data;

  // setup load text
  if (!font.loadFromFile(graphics::font_dir + "arial.ttf")){
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
    draw_universe();
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

  reload_data(data);

  cout << "pre_step: end: game data has " << data.ships.size() << " ships" << endl;

  window.setView(sf::View(sf::FloatRect(0, 0, settings.width, settings.height)));

  return true;
}

// ****************************************
// CHOICE STEP
// ****************************************

void st3::client::game::choice_step(){
  bool done = false;
  sf::Packet pq, pr;
  int count = 0;
  sf::Font font;
  sf::Text text;

  // setup load text
  if (!font.loadFromFile(graphics::font_dir + "arial.ttf")){
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

  while (!done){
    sf::Event event;
    while (window.pollEvent(event)){
      if (event.type == sf::Event::Closed) exit(-1);
      done |= choice_event(event);
    }

    controls();

    window.clear();
    draw_universe();
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
    draw_universe();
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
  int idx = -1;
  int loaded = 0;
  vector<game_data> g(settings.frames_per_round);

  thread t(load_frames, socket, ref(g), ref(loaded));

  while (!done){
    done |= !window.isOpen();
    done |= idx == settings.frames_per_round - 1;

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

    controls();

    window.clear();
    draw_universe();
    window.display();

    playing &= idx < loaded - 1;

    if (playing){
      idx++;
      reload_data(g[idx]);
    }

    sf::sleep(sf::milliseconds(100));    
  }

  t.join();
}

// ****************************************
// DATA HANDLING
// ****************************************

choice st3::client::game::build_choice(){
  choice c;
  for (auto x : command_selectors){
    c.commands.push_back(*x.second);
  }

  return c;
}

void st3::client::game::reload_data(const game_data &g){
  clear_selectors();
  
  ships = g.ships;
  players = g.players;
  settings = g.settings;
  
  for (auto x : g.fleets){
    source_t key = identifier::make(identifier::fleet, x.first);
    sf::Color col = graphics::sfcolor(players[x.second.owner].color);
    entity_selectors[key] = new fleet_selector(x.second, col, x.second.owner == socket.id);
  }

  for (auto x : g.solars){
    source_t key = identifier::make(identifier::solar, x.first);
    sf::Color col = x.second.owner > -1 ? graphics::sfcolor(players[x.second.owner].color) : graphics::solar_neutral;
    entity_selectors[key] = new solar_selector(x.second, col, x.second.owner == socket.id);
  }
}

// ****************************************
// COMMAND MANIPULATION
// ****************************************

bool st3::client::game::command_exists(command c){
  for (auto x : command_selectors){
    if (c == (command)*x.second) return true;
  }
  
  return false;
}

void st3::client::game::add_command(command c, point from, point to){
  if (command_exists(c)) return;
  idtype id = comid++;
  source_t key = identifier::make(identifier::command, id);
  command_selector *cs = new command_selector(c, from, to);

  // add command selector to entity selectors
  entity_selectors[key] = cs;
  
  // add command to command selectors
  command_selectors[key] = cs;

  // add command selector key to list of the source entity's children
  entity_commands[c.source].insert(key);
}

// remove command selector and associated command
void st3::client::game::remove_command(source_t key){
  if (identifier::get_type(key).compare(identifier::command)){
    cout << "remove command: is not a command: " << key << endl;
    exit(-1);
  }

  if (entity_selectors.count(key) && command_selectors.count(key)){
    command_selector *s = command_selectors[key];

    // remove this command from it's parent's list
    if (!entity_commands.count(s -> source)){
      cout << "remove_command: no entity_commands with source key " << s -> source << endl;
      exit(-1);
    }
    entity_commands[s -> source].erase(key);

    // remove this command's child commands and list thereof
    for (auto x : entity_commands[key]) remove_command(x);
    entity_commands.erase(key);

    // remove this command's selector
    entity_selectors.erase(key);
    command_selectors.erase(key);
    delete s;
  }else{
    cout << "remove command: " << key << ": not found!" << endl;
    exit(-1);
  }
}

// key must reference a command selector
void st3::client::game::increment_command(source_t key, int delta){
  command_selector *s = command_selectors[key];

  if (!s) return;

  // compute allocated quantity of parent entity
  if (!entity_commands.count(s -> source)){
    cout << "game::increment_command: " << s -> source << ": no entity commands" << endl;
    exit(-1);
  }

  int sum = 0;
  for (auto x : entity_commands[s -> source]){
    command_selector *c = command_selectors[x];
    if (!c){
      cout << "increment_command: " << key << ": missing sibling command: " << x << endl;
      exit(-1);
    }
    sum += c -> quantity;
  }

  entity_selector *source = entity_selectors[s -> source];
  if (!source){
    cout << "increment_command: " << key << ": missing source: " << s -> source << endl;
    exit(-1);
  }
  if (sum + delta <= source -> get_quantity()){
    s -> quantity += delta;
  }
}

// ****************************************
// SELECTOR MANIPULATION
// ****************************************

void st3::client::game::clear_selectors(){
  comid = 0;

  for (auto x : entity_selectors){
    delete x.second;
  }

  entity_selectors.clear();
  entity_commands.clear();
}

void st3::client::game::deselect_all(){
  for (auto x : entity_selectors) x.second -> selected = false;
}

// ****************************************
// EVENT HANDLING
// ****************************************

void st3::client::game::area_select(){
  sf::FloatRect rect = fixrect(srect);

  if (!add2selection()) deselect_all();

  cout << "area select" << endl;
  
  for (auto x : entity_selectors){
    x.second -> selected = x.second -> owned && x.second -> area_selectable && x.second -> inside_rect(rect);
  }

}

void st3::client::game::command2point(point p){
  command c;
  point from, to;
  c.target = identifier::make(identifier::point, p);
  to = p;

  cout << "command to point: " << p.x << "," << p.y << endl;
  for (auto x : entity_selectors){
    if (x.second -> selected){
      cout << "command2point: adding " << x.first << endl;
      c.source = x.first;
      from = x.second -> get_position();
      add_command(c, from, to);
    }
  }
}

void st3::client::game::command2entity(source_t key){
  if (!entity_selectors.count(key)){
    cout << "command2entity: invalid key: " << key << endl;
    exit(-1);
  }

  command c;
  point from, to;
  entity_selector *s = entity_selectors[key];
  c.target = key;
  to = s -> get_position();

  cout << "command2entity: " << key << endl;

  for (auto x : entity_selectors){
    if (x.second != s && x.second -> selected){
      cout << "command2entity: adding " << x.first << endl;
      c.source = x.first;
      from = x.second -> get_position();
      add_command(c, from, to);
    }
  }
}

source_t st3::client::game::entity_at(point p){
  float max_click_dist = 50;
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

  cout << "entity at: " << p.x << "," << p.y << ": " << key << endl;

  return key;
}

void st3::client::game::target_at(point p){
  source_t key = entity_at(p);

  if (entity_selectors.count(key)){
    cout << "target at: " << key << endl;
    command2entity(key);
  }else{
    cout << "target at: point " << p.x << "," << p.y << endl;
    command2point(p);
  }
}

void st3::client::game::select_at(point p){
  cout << "select at: " << p.x << "," << p.y << endl;
  source_t key = entity_at(p);
  auto it = entity_selectors.find(key);
 
  if (!add2selection()) deselect_all();

  if (it != entity_selectors.end() && it -> second -> owned){
    it -> second -> selected = !(it -> second -> selected);
    cout << "selector found: " << it -> first << " -> " << it -> second -> selected << endl;
  }
}

// return true to signal choice step done
bool st3::client::game::choice_event(sf::Event e){
  point p;
  auto i = entity_selectors.begin();
  sf::View view = window.getView();

  switch (e.type){
  case sf::Event::MouseButtonPressed:
    if (e.mouseButton.button == sf::Mouse::Left){
      point p = window.mapPixelToCoords(sf::Vector2i(e.mouseButton.x, e.mouseButton.y));
      area_select_active = true;
      srect = sf::FloatRect(p.x, p.y, 0, 0);
      cout << "set area UL to: " << srect.left << "x" << srect.top << endl;
    }
    break;
  case sf::Event::MouseButtonReleased:
    p = window.mapPixelToCoords(sf::Vector2i(e.mouseButton.x, e.mouseButton.y));
    if (e.mouseButton.button == sf::Mouse::Left){
      if (srect.width > 5 || srect.height > 5){
	area_select();
      } else {
	select_at(p);
      }
    }else if (e.mouseButton.button == sf::Mouse::Right){
      target_at(p);
    }

    // clear selection rect
    area_select_active = false;
    srect = sf::FloatRect(0, 0, 0, 0);
    break;
  case sf::Event::MouseMoved:
    if (area_select_active){
      p = window.mapPixelToCoords(sf::Vector2i(e.mouseMove.x, e.mouseMove.y));
      srect.width = p.x - srect.left;
      srect.height = p.y - srect.top;
      cout << "update srect size at " << p.x << "x" << p.y << " to " << srect.width << "x" << srect.height << endl;
    }
    break;
  case sf::Event::MouseWheelMoved:
    // increment selected commands
    for (auto x : entity_selectors){
      if (x.second -> selected && x.second -> isa(identifier::command)){
	cout << "incrementing command " << identifier::get_id(x.first) << " by " << e.mouseWheel.delta << endl;
	increment_command(x.first, e.mouseWheel.delta);
      }
    }
    
    break;
  case sf::Event::KeyPressed:
    switch (e.key.code){
    case sf::Keyboard::Space:
      return true;
    case sf::Keyboard::Delete:
      i = entity_selectors.begin();
      while (i != entity_selectors.end()){
	if ((i -> second -> isa(identifier::command)) && i -> second -> selected){
	  source_t key = i -> first;
	  i++;
	  remove_command(key);
	}else{
	  i++;
	}
      }
      break;
    case sf::Keyboard::O:
      view.zoom(1.2);
      break;
    case sf::Keyboard::I:
      view.zoom(1 / 1.2);
      break;
    }
    break;
  };

  window.setView(view);
  
  return false;
}

void st3::client::game::controls(){
  // if (!window.hasFocus()) {
  //   vel = point(0,0);
  //   return;
  // }

  static point vel(0,0);
  sf::View view = window.getView();
  float s = view.getSize().x / settings.width;

  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)){
    vel.x -= 5 * s;
  } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)){
    vel.x += 5 * s;
  } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)){
    vel.y -= 5 * s;
  } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)){
    vel.y += 5 * s;
  }

  vel = utility::scale_point(vel, 0.8);
  
  view.move(vel);
  window.setView(view);
}

// ****************************************
// GRAPHICS
// ****************************************

void st3::client::game::draw_universe(){
  sf::Color col;
  sf::Vertex svert[4];

  // draw ships
  for (auto x : ships) {
    ship s = x.second;
    if (!s.was_killed){
      col = graphics::sfcolor(players[s.owner].color);
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

  // draw other entities
  for (auto x : entity_selectors){
    x.second -> draw(window);
  }

  if (area_select_active && srect.width && srect.height){
    // draw selection rect
    sf::RectangleShape r;
    r.setSize(sf::Vector2f(srect.width, srect.height));
    r.setPosition(sf::Vector2f(srect.left, srect.top));
    r.setFillColor(sf::Color(250,250,250,50));
    r.setOutlineColor(sf::Color(80, 120, 240, 200));
    r.setOutlineThickness(1);
    window.draw(r);
  }
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
