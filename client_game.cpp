#include <iostream>
#include <thread>
#include <queue>

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

idtype waypoint::id_counter = 0;

sf::FloatRect fixrect(sf::FloatRect r);
bool add2selection();
bool ctrlsel();

// ****************************************
// GAME STEPS
// ****************************************

game::game(){
  comgui = 0;
}

void game::clear_guis(){
  if (comgui) delete comgui;
  comgui = 0;
}

void st3::client::game::run(){
  bool proceed = true;
  area_select_active = false;
  window.setView(sf::View(sf::FloatRect(0, 0, settings.width, settings.height)));

  solar::development::initialize();

  // game loop
  while (proceed){
    clear_guis();
    proceed &= pre_step();
    choice_step();
    simulation_step();
  }
}

// ****************************************
// PRE_STEP
// ****************************************

bool st3::client::game::pre_step(){
  int done = client::query_ask;
  sf::Packet packet, pq;
  sf::Text text;
  game_data data;

  text.setFont(graphics::default_font); 
  text.setString("loading game data...");
  text.setCharacterSize(24);
  text.setColor(sf::Color(200,200,200));
  // text.setStyle(sf::Text::Underlined);
  text.setPosition(10, 10);

  pq << protocol::game_round;

  cout << "pre_step: start: game data has " << data.ships.size() << " ships" << endl;
  
  // todo: handle response: complete
  thread t(query, 
	   socket, 
	   ref(pq),
	   ref(packet), 
	   ref(done));

  while (!done){
    if (!window.isOpen()) done |= client::query_finished;

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

  if (done & query_finished){
    cout << "pre_step: finished" << endl;
    exit(0);
  }

  if (!(packet >> data)){
    cout << "pre_step: failed to deserialize game_data" << endl;
    exit(-1);
  }

  reload_data(data);

  cout << "pre_step: end: game data has " << data.ships.size() << " ships" << endl;

  return true;
}

// ****************************************
// CHOICE STEP
// ****************************************

void st3::client::game::choice_step(){
  int done = client::query_ask;
  sf::Packet pq, pr;
  sf::Text text;

  cout << "choice_step: start" << endl;

  text.setFont(graphics::default_font); 
  text.setString("make your choice");
  text.setCharacterSize(24);
  text.setColor(sf::Color(200,200,200));
  text.setPosition(10, 10);

  // CREATE THE CHOICE (USER INTERFACE)

  while (!done){
    sf::Event event;
    while (window.pollEvent(event)){
      if (event.type == sf::Event::Closed){
	done |= client::query_finished;
      }else{
	done |= choice_event(event);
      }
    }

    controls();

    window.clear();
    draw_universe();
    window.draw(text);
    window.display();

    sf::sleep(sf::milliseconds(100));
  }

  if (done & client::query_finished){
    cout << "choice_step: finishded" << endl;
    exit(0);
  }

  // SEND THE CHOICE TO SERVER

  text.setString("sending choice to server...");
  choice c = build_choice();
  done = client::query_ask;
  pq << protocol::choice;
  pq << c;

  thread t(query, 
	   socket, 
	   ref(pq),
	   ref(pr), 
	   ref(done));

  cout << "choice step: sending" << endl;

  while (!done){
    if (!window.isOpen()) done |= client::query_finished;

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

  if (done & client::query_finished){
    cout << "choice send: finished" << endl;
    exit(0);
  }

  clear_guis();
  deselect_all();

  cout << "choice step: waiting for query thread" << endl;
  t.join();
  cout << "choice step: complete" << endl;
}

// ****************************************
// SIMULATION STEP
// ****************************************

void st3::client::game::simulation_step(){
  int done = client::query_ask;
  bool playing = true;
  int idx = -1;
  int loaded = 0;
  vector<game_data> g(settings.frames_per_round);
  sf::Text text;

  text.setFont(graphics::default_font); 
  text.setString("evolution: playing");
  text.setCharacterSize(24);
  text.setColor(sf::Color(200,200,200));
  text.setPosition(10, 10);

  thread t(load_frames, socket, ref(g), ref(loaded));

  while (!done){
    if (!window.isOpen()) done |= client::query_finished;
    if (idx == settings.frames_per_round - 1) done |= client::query_proceed;

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
    text.setString(playing ? "evolution: playing" : "evolution: paused");

    window.clear();
    draw_universe();
    window.draw(text);
    window.display();

    playing &= idx < loaded - 1;

    if (playing){
      idx++;
      reload_data(g[idx]);
    }

    sf::sleep(sf::milliseconds(100));    
  }

  t.join();

  if (done & client::query_finished){
    cout << "simulation step: finished" << endl;
    exit(0);
  }
}

// ****************************************
// DATA HANDLING
// ****************************************

source_t game::add_waypoint(point p){
  source_t k = identifier::make(identifier::waypoint, to_string(socket.id) + "#" + to_string(waypoint::id_counter++));

  waypoint w;
  w.position = p;

  entity_selectors[k] = new waypoint_selector(w, graphics::sfcolor(players[socket.id].color));

  cout << "added waypoint " << k << endl;

  return k;
}

command st3::client::game::build_command(idtype key){
  if (!command_selectors.count(key)){
    cout << "build_command: not found: " << key << endl;
    exit(-1);
  }

  return (command)*command_selectors[key];
}

choice st3::client::game::build_choice(){
  choice c;
  cout << "build choice:" << endl;
  for (auto x : entity_selectors){
    if (x.second -> isa(identifier::waypoint)){
      waypoint_selector *ws = (waypoint_selector*)x.second;
      waypoint w = (waypoint)*ws;
      w.pending_commands.clear();
      for (auto k : x.second -> commands) {
	if (!command_selectors.count(k)){
	  cout << "build_choice: error: command_selector " << k << " is mising!" << endl;
	}
	w.pending_commands.push_back((command)*command_selectors[k]);
	command com = w.pending_commands.back();
	cout << x.first << ": command " << k << " from " << com.source << " to " << com.target << ": " << endl << "ships: ";
	for (auto s : com.ships){
	  cout << s << ", ";
	}
	cout << endl;
      }
      c.waypoints[identifier::get_string_id(x.first)] = w;
    }else{
      for (auto y : x.second -> commands){
	c.commands[x.first].push_back(build_command(y));
	cout << "adding command " << y << " with key " << x.first << ", source " << c.commands[x.first].back().source << " and target " << c.commands[x.first].back().target << endl;
      }
    }
  }

  return c;
}

list<idtype> game::incident_commands(source_t key){
  list<idtype> res;

  for (auto x : command_selectors){
    if (!x.second -> target.compare(key)){
      res.push_back(x.first);
    }
  }

  return res;
}

void st3::client::game::reload_data(game_data &g){
  clear_selectors();
  
  ships = g.ships;
  players = g.players;
  settings = g.settings;

  cout << "game::reload_data" << endl;

  // create entities: fleets, solars and waypoints
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

  for (auto x : g.waypoints){
    // owned?
    if (identifier::get_waypoint_owner(x.first) == socket.id){
      source_t key = identifier::make(identifier::waypoint, x.first);
      sf::Color col = graphics::sfcolor(players[socket.id].color);
      waypoint_selector *ws = new waypoint_selector(x.second, col);
      entity_selectors[key] = ws;
      ws -> ships = ws -> landed_ships;
      cout << " -> added waypoint " << x.first << endl;
    }
  }

  // propagate remaining ships through waypoints
  // fleets are sources
  set<waypoint_selector*> buf;
  for (auto x : entity_selectors){
    if (x.second -> owned && x.second -> isa(identifier::fleet)){
      fleet_selector *fs = (fleet_selector*)x.second;
      if (!fs -> is_idle()){
	command c = fs -> com;

	point from, to;
	if (g.target_position(c.source, from) && g.target_position(c.target, to)){
	  // assure we don't assign ships which have been killed
	  c.ships = c.ships & fs -> ships;
	  cout << " -> adding fleet fommand from " << c.source << " to " << c.target << " with " << c.ships.size() << " ships." << endl;

	  add_command(c, from, to, false);
	  if (entity_selectors[c.target] -> isa(identifier::waypoint)){
	    buf.insert((waypoint_selector*)entity_selectors[c.target]);
	  }
	}else{
	  cout << "client side target position miss!" << endl;
	}
      }
    }
  }

  // move to queue for better propagation order
  queue<waypoint_selector*> q;
  for (auto x : buf) q.push(x);

  while(!q.empty()){
    waypoint_selector *s = q.front();
    q.pop();
    for (auto c : s -> pending_commands){
      entity_selector *t = entity_selectors[c.target];
      idtype cid = command_id(c);
      if (cid > -1){
	// command selector already added
	command_selector *cs = command_selectors[cid];
	
	// add new ships to command selector
	cs -> ships += c.ships & s -> ships;
	cout << "filling cs " << cid << " with " << cs -> ships.size() << " ships" << endl;
	
	// add new ships to target waypoint
	if (t -> isa(identifier::waypoint)){
	  waypoint_selector *ws = (waypoint_selector*)t;
	  ws -> ships += cs -> ships;
	}
      }else{
	point from, to;
	if (g.target_position(c.source, from) && g.target_position(c.target, to)){
	  c.ships = c.ships & s -> ships;
	  add_command(c, from, to, false);
	}else{
	  cout << "client side target position miss!" << endl;
	}
      }

      if (t -> isa(identifier::waypoint)){
	q.push((waypoint_selector*)t);
      }
    }
  }
}

// ****************************************
// COMMAND MANIPULATION
// ****************************************

bool st3::client::game::command_exists(command c){
  cout << "command_exists: start" << endl;
  return command_id(c) > -1;
}

idtype game::command_id(command c){
  for (auto x : command_selectors){
    cout << " -> checking: " << x.first << endl;
    if (c == (command)*x.second) return x.first;
  }

  return -1;
}

void st3::client::game::add_command(command c, point from, point to, bool fill_ships){
  if (command_exists(c)) return;
  idtype id = comid++;
  command_selector *cs = new command_selector(c, from, to);
  entity_selector *s = entity_selectors[c.source];
  entity_selector *t = entity_selectors[c.target];

  // check waypoint ancestors
  if (waypoint_ancestor_of(c.target, c.source)){
    cout << "add_command: circular waypoint graphs forbidden!" << endl;
    return;
  }

  cout << "add command: " << id << " from " << c.source << " to " << c.target << endl;
  
  // add command to command selectors
  command_selectors[id] = cs;

  // add command selector key to list of the source entity's children
  s -> commands.insert(id);

  // add ships to command
  set<idtype> ready_ships = s -> get_ready_ships();
  cout << "ready ships: " << ready_ships.size() << endl;
  if (fill_ships) cs -> ships = ready_ships;
  s -> allocated_ships += cs -> ships;

  // add ships to waypoint
  if (t -> isa(identifier::waypoint)){
    waypoint_selector *ws = (waypoint_selector*)t;
    ws -> ships += cs -> ships;
  }

  cout << "added command: " << id << endl;
}

void game::recursive_waypoint_deallocate(source_t wid, set<idtype> a){
  entity_selector *es = entity_selectors[wid];
  if (!es -> isa(identifier::waypoint)) return;
  waypoint_selector *s = (waypoint_selector*)es;

  cout << "RWD start" << endl;
  // check if there are still incoming commands

  if (incident_commands(wid).empty()){
    cout << "removing waypoint " << wid << endl;
    // remove waypoint
    auto buf = s -> commands;
    for (auto c : buf){
      cout << " .. remove command: " << c << endl;
      remove_command(c);
    }
    entity_selectors.erase(wid);
    delete s;
    return;
  }

  if (a.empty()) return;

  s -> ships -= a;
  s -> allocated_ships -= a;

  cout << "removing " << a.size() << " ships from waypoint " << wid << endl;
  
  for (auto c : s -> commands){
    command_selector *cs = command_selectors[c];
    
    cout << " .. removing ships from command " << c << endl;
    // ships to remove from this command
    set<idtype> delta = cs -> ships & a;
    cs -> ships -= delta;

    cout << " .. recursive call RWD on " << cs -> target << endl;
    recursive_waypoint_deallocate(cs -> target, delta);
  }
}

bool game::waypoint_ancestor_of(source_t ancestor, source_t child){
  // only waypoints can have this relationship
  if (identifier::get_type(ancestor).compare(identifier::waypoint) || identifier::get_type(child).compare(identifier::waypoint)){
    return false;
  }

  // if the child is the ancestor, it has been found!
  if (!ancestor.compare(child)) return true;

  // if the ancestor is ancestor to a parent, it is ancestor to the child
  for (auto x : incident_commands(child)){
    if (waypoint_ancestor_of(ancestor, command_selectors[x] -> source)) return true;
  }

  return false;
}

// remove command selector and associated command
void st3::client::game::remove_command(idtype key){
  cout << "remove command: " << key << endl;

  if (command_selectors.count(key)){
    command_selector *cs = command_selectors[key];
    entity_selector *s = entity_selectors[cs -> source];
    set<idtype> cships = cs -> ships;
    source_t target_key = cs -> target;

    cout << " .. deleting; source = " << cs -> source << endl;
    // remove this command's selector
    command_selectors.erase(key);
    delete cs;

    // remove this command from it's source's list
    s -> commands.erase(key);

    // deallocate ships from source
    s -> allocated_ships -= cships;

    cout << " .. calling RWD" << endl;
    // remove ships from waypoint
    recursive_waypoint_deallocate(target_key, cships);

    // remove comgui
    if (comgui) delete comgui;
    comgui = 0;

    cout << "command removed: " << key << endl;
  }else{
    cout << "remove command: " << key << ": not found!" << endl;
    exit(-1);
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

  for (auto x : command_selectors){
    delete x.second;
  }

  entity_selectors.clear();
  command_selectors.clear();
}

void st3::client::game::deselect_all(){
  for (auto x : entity_selectors) x.second -> selected = false;
  for (auto x : command_selectors) x.second -> selected = false;
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
  // float max_click_dist = 50;
  // float dmin = max_click_dist;
  float d;
  source_t key = "";
  int q = entity_selector::queue_max;

  cout << "entity_at:" << endl;

  // find closest entity to p
  for (auto x : entity_selectors){
    cout << "checking entity: " << x.first << endl;
    if (x.second -> contains_point(p, d) && x.second -> queue_level < q){
      q = x.second -> queue_level;
      key = x.first;
      cout << "entity_at: k = " << key << ", q = " << x.second -> queue_level << endl;
    }else if (x.second -> contains_point(p, d)){
      cout << "entity_at: queued out: " << x.first << ", q = " << x.second -> queue_level << endl;
    }
  }

  cout << "entity at: " << p.x << "," << p.y << ": " << key << endl;
  if (key.length() && key.compare(entity_selector::last_selected)){
    entity_selectors[key] -> queue_level = (entity_selectors[key] -> queue_level + 1) % entity_selector::queue_max;
    entity_selector::last_selected = key;
  }

  return key;
}


idtype st3::client::game::command_at(point p){
  float max_click_dist = 50;
  float dmin = max_click_dist;
  float d;
  idtype key = -1;

  // find closest entity to p
  for (auto x : command_selectors){
    if (x.second -> contains_point(p, d) && d < dmin){
      dmin = d;
      key = x.first;
    }
  }

  cout << "command at: " << p.x << "," << p.y << ": " << key << endl;

  return key;
}

void st3::client::game::target_at(point p){
  source_t key = entity_at(p);

  if (entity_selectors.count(key)){
    cout << "target at: " << key << endl;
    command2entity(key);
  }else{
    cout << "target at: point " << p.x << "," << p.y << endl;
    if (count_selected()){
      command2entity(add_waypoint(p));
    }
  }
}

bool st3::client::game::select_at(point p){
  cout << "select at: " << p.x << "," << p.y << endl;
  source_t key = entity_at(p);
  auto it = entity_selectors.find(key);
 
  if (!add2selection()) deselect_all();

  if (it != entity_selectors.end() && it -> second -> owned){
    it -> second -> selected = !(it -> second -> selected);
    cout << "selector found: " << it -> first << " -> " << it -> second -> selected << endl;

    if (it -> second -> selected){
      cout << "ships at " << it -> first << endl;
      for (auto x : it -> second -> get_ships()){
	cout << x << ", ";
      }
      cout << endl;
    }
    return true;
  }

  return false;
}

bool st3::client::game::select_command_at(point p){
  cout << "select at: " << p.x << "," << p.y << endl;
  idtype key = command_at(p);
  auto it = command_selectors.find(key);
 
  if (!add2selection()) deselect_all();

  if (it != command_selectors.end()){
    it -> second -> selected = !(it -> second -> selected);
    cout << "command found: " << it -> first << " -> " << it -> second -> selected << endl;

    // setup command gui
    hm_t<idtype, ship> ready_ships;
    
    for (auto x : entity_selectors[it -> second -> source] -> get_ready_ships()){
      ready_ships[x] = ships[x];
    }
    
    for (auto x : it -> second -> ships){
      ready_ships[x] = ships[x];
    }

    comgui = new command_gui(it -> first, window, 
			     ready_ships,
			     it -> second -> ships,
			     utility::scale_point(it -> second -> from + it -> second -> to, 0.5), 
			     graphics::sfcolor(players[socket.id].color));

    return true;
  }

  return false;
}

int game::count_selected(){
  int sum = 0;
  for (auto x : entity_selectors) sum += x.second -> selected;
  return sum;
}

// return true to signal choice step done
int st3::client::game::choice_event(sf::Event e){
  point p;
  sf::View view = window.getView();

  if (comgui && comgui -> handle_event(e)){
    // handle comgui effects

    command_selector *cs = command_selectors[comgui -> comid];
    entity_selector *s = entity_selectors[cs -> source];
    entity_selector *t = entity_selectors[cs -> target];

    // check if target is a waypoint
    if (t -> isa(identifier::waypoint)){
      waypoint_selector *ws = (waypoint_selector*)t;

      // compute added or removed ships
      set<idtype> removed = cs -> ships - comgui -> allocated;
      set<idtype> added = comgui -> allocated - cs -> ships;

      if (!added.empty()){
	ws -> ships += added;
      }

      if (!removed.empty()){
	recursive_waypoint_deallocate(cs -> target, removed);
      }
    }

    // set command selector's ships
    cs -> ships = comgui -> allocated;
    
    // set source entity's allocated ships
    s -> allocated_ships += comgui -> allocated;

    // unset source entity's non-allocated ships
    s -> allocated_ships -= comgui -> cached;

    return client::query_ask;
  }

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
    clear_guis();

    p = window.mapPixelToCoords(sf::Vector2i(e.mouseButton.x, e.mouseButton.y));
    if (e.mouseButton.button == sf::Mouse::Left){
      if (abs(srect.width) > 5 || abs(srect.height) > 5){
	area_select();
      } else if (!select_command_at(p)){
	select_at(p);
      }
    } else if (e.mouseButton.button == sf::Mouse::Right){
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
    break;
  case sf::Event::KeyPressed:
    switch (e.key.code){
    case sf::Keyboard::Space:
      cout << "choice_event: proceed" << endl;
      return client::query_proceed;
    case sf::Keyboard::Delete:
      for (auto i : list<pair<idtype, command_selector*> >(command_selectors.begin(), command_selectors.end())){
	if (i.second -> selected){
	  remove_command(i.first);
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
  
  return client::query_ask;
}

void st3::client::game::controls(){
  static point vel(0,0);
  if (!window.hasFocus()) {
    vel = point(0,0);
    return;
  }

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

  // draw source/target entities
  for (auto x : entity_selectors) x.second -> draw(window);

  // draw commands
  for (auto x : command_selectors) x.second -> draw(window);

  // draw ships in fleets
  for (auto x : ships){
    if (x.second.fleet_id > -1){
      sf::Color col = graphics::sfcolor(players[x.second.owner].color);
      graphics::draw_ship(window, x.second, col);
    }
  }

  if (area_select_active && srect.width && srect.height){
    // draw selection rect
    sf::RectangleShape r = utility::build_rect(srect);
    r.setFillColor(sf::Color(250,250,250,50));
    r.setOutlineColor(sf::Color(80, 120, 240, 200));
    r.setOutlineThickness(1);
    window.draw(r);
  }

  // draw command gui
  if (comgui) comgui -> draw();
}

// ****************************************
// UTILITY FUNCTIONS
// ****************************************

bool add2selection(){
  return sf::Keyboard::isKeyPressed(sf::Keyboard::LShift);
}

bool ctrlsel(){
  return sf::Keyboard::isKeyPressed(sf::Keyboard::LControl);
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
