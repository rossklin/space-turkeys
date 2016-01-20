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
using namespace client;
using namespace graphics;

sf::FloatRect fixrect(sf::FloatRect r);
bool add2selection();
bool ctrlsel();

// ****************************************
// GAME STEPS
// ****************************************

game::game(){
  comgui = 0;
  targui = 0;
  selector_queue = 1;

  default_event_handler = [this](sf::Event e) -> int {
    if (e.type == sf::Event::Closed) {
      window.close();
      return query_aborted;
    }else{
      return 0;
    }
  };

  default_body = [this] () -> int {
    if (window.isOpen()) {return 0;} else {return query_aborted;}
  };
}

void game::clear_guis(){
  if (comgui) delete comgui;
  comgui = 0;
  if (targui) delete targui;
  targui = 0;
}

void st3::client::game::run(){  
  
  bool proceed = true;
  bool first = true;
  area_select_active = false;
  view_game = sf::View(sf::FloatRect(0, 0, settings.width, settings.height));
  view_minimap = view_game;
  view_minimap.setViewport(sf::FloatRect(0.01, 0.71, 0.28, 0.28));
  view_window = window.getDefaultView();


  // construct interface
  window.setView(view_window);
  interface::desktop = new interface::main_interface(window.getSize(), players[socket.id].research_level);

  // game loop
  while (true){
    clear_guis();

    if (!pre_step()) break;

    if (first){
      // initialize position at home solar
      first = false;
      for (auto i : entity_selectors){
	if (i.second -> isa(identifier::solar) && i.second -> owned){
	  view_game.setCenter(i.second -> get_position());
	  view_game.setSize(point(25 * settings.solar_maxrad, 25 * settings.solar_maxrad));
	}
      }
    }

    choice_step();

    simulation_step();
  }

  delete interface::desktop;
  interface::desktop = 0;
}

// ****************************************
// PRE_STEP
// ****************************************

bool st3::client::game::pre_step(){
  int done = query_query;
  sf::Packet packet, pq;
  game_data data;

  message = "loading game data...";
  pq << protocol::game_round;

  cout << "pre_step: start: game data has " << data.ships.size() << " ships" << endl;
  
  // todo: handle response: complete
  thread t(query, 
	   socket, 
	   ref(pq),
	   ref(packet), 
	   ref(done));

  done = window_loop(done, default_event_handler, default_body);

  cout << "pre_step: waiting for com thread to finish..." << endl;
  t.join();

  if (done & query_game_complete){
    cout << "pre_step: finished" << endl;

    string winner;
    if (!(packet >> winner)){
      cout << "server does not declare winner!" << endl;
      return false;
    }

    message = "winner: " + winner;

    int i = 0;

    auto body = [this, &i]() -> int {
      if (i++ < 40 && window.isOpen()){
	return 0;
      }else{
	return query_aborted;
      }
    };
    
    done = window_loop(done, default_event_handler, body);
    
    return false;
  }

  if (done & query_aborted){
    cout << "pre step: aborted" << endl;
    return false;
  }

  if (!(packet >> data)){
    cout << "pre_step: failed to deserialize game_data" << endl;
    return false;
  }

  reload_data(data);

  cout << "pre_step: end: game data has " << data.ships.size() << " ships" << endl;

  return true;
}

// ****************************************
// CHOICE STEP
// ****************************************

void st3::client::game::choice_step(){
  int done = query_query;
  sf::Packet pq, pr;

  cout << "choice_step: start" << endl;

  message = "make your choice";

  // CREATE THE CHOICE (USER INTERFACE)

  auto event_handler = [this](sf::Event e) -> int {
    int done = 0;
    if (e.type == sf::Event::Closed){
      window.close();
    }else{
      done |= choice_event(e);
      if (!done) interface::desktop -> HandleEvent(e);
    }
    return done;
  };

  done = window_loop(done, event_handler, default_body);

  if (done & query_game_complete){
    cout << "choice_step: finishded" << endl;
    exit(0);
  }

  // SEND THE CHOICE TO SERVER

  message = "sending choice to server...";

  // add commands to choice
  choice::choice c = build_choice(interface::desktop -> response);
  done = query_query;
  pq << protocol::choice;
  pq << c;

  thread t(query, 
	   socket, 
	   ref(pq),
	   ref(pr), 
	   ref(done));

  cout << "choice step: sending" << endl;

  done = window_loop(done, default_event_handler, default_body);

  if (done & query_game_complete){
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
  int done = query_query;
  bool playing = true;
  int idx = -1;
  int loaded = 0;
  vector<game_data> g(settings.frames_per_round);
  thread t(load_frames, socket, ref(g), ref(loaded));

  auto event_handler = [this, &playing] (sf::Event e) -> int {
    switch (e.type){
    case sf::Event::Closed:
    window.close();
    return query_aborted;
    case sf::Event::KeyPressed:
    if (e.key.code == sf::Keyboard::Space){
      playing = !playing;
    }
    return 0;
    }
  };

  auto body = [&,this] () -> int {
    if (!window.isOpen()) return query_aborted;
    if (idx == settings.frames_per_round - 1) return query_accepted;

    message = "evolution: " + to_string((100 * idx) / settings.frames_per_round) + " %" + (playing ? "" : "(paused)");

    playing &= idx < loaded - 1;

    if (playing){
      idx++;
      reload_data(g[idx]);
    }
    return 0;
  };

  done = window_loop(done, event_handler, body);

  t.join();

  if (done & (query_game_complete | query_aborted)){
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

  entity_selectors[k] = new waypoint_selector(w, sfcolor(players[socket.id].color));

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

 choice::choice st3::client::game::build_choice(choice::choice c){
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

void st3::client::game::add_fixed_stars (point position, float vision) {
  float r = vision + grid_size;
  
  for (float p = -r; p < r; p += grid_size) {
    float ymax = sqrt (r * r - p * p);
    for (float j = -ymax; j < ymax; j += grid_size) {
      int grid_x = round ((position.x + p) / grid_size);
      int grid_y = round ((position.y + j) / grid_size);
      pair<int, int> grid_index (grid_x, grid_y);
      
      for (int i = rand () % 3; i >= 0; i--) {
        if (known_universe.count (grid_index) == 0) {
          point star_position;
          star_position.x = grid_index.first * grid_size + utility::random_uniform () * grid_size;
          star_position.y = grid_index.second * grid_size + utility::random_uniform () * grid_size;
          
          fixed_star star(star_position);
          
          if (utility::l2norm(star_position - position) < vision) {
            fixed_stars.push_back (star);
          } else {
            hidden_stars.push_back (star);
          }
          
          known_universe.insert (grid_index);
        }
      }
    }
  }
  
  auto i = hidden_stars.begin();
  while (i != hidden_stars.end()) {
    fixed_star star = *i;
    i++;
    if (utility::l2norm (star.position - position) < vision) {
      fixed_stars.push_back (star);
      hidden_stars.remove (star);
    }
  }
}

void st3::client::game::reload_data(game_data &g){
  cout << "reload data:" << endl;
  cout << " -> initial entities: " << endl;
  for (auto x : entity_selectors) cout << x.first << endl;

  clear_selectors();
  
  players = g.players;
  settings = g.settings;
  cout << " -> post clear entities: " << endl;
  for (auto x : entity_selectors) cout << x.first << endl;

  cout << "dt = " << settings.dt << endl;

  // update entities: fleets, solars and waypoints
  for (auto x : g.fleets){
    source_t key = identifier::make(identifier::fleet, x.first);
    sf::Color col = sfcolor(players[x.second.owner].color);
    if (entity_selectors.count(key)) delete entity_selectors[key];
    entity_selectors[key] = new fleet_selector(x.second, col, x.second.owner == socket.id);
    entity_selectors[key] -> seen = true;
    if (x.second.owner == socket.id) add_fixed_stars (x.second.position, x.second.vision);
    
    cout << " -> update fleet " << x.first << endl;
  }

  for (auto x : g.solars){
    source_t key = identifier::make(identifier::solar, x.first);
    sf::Color col = x.second.owner > -1 ? sfcolor(players[x.second.owner].color) : solar_neutral;
    if (entity_selectors.count(key)) delete entity_selectors[key];
    entity_selectors[key] = new solar_selector(x.second, col, x.second.owner == socket.id);
    entity_selectors[key] -> seen = true;
    if (x.second.owner == socket.id) add_fixed_stars (x.second.position, x.second.compute_vision());

    cout << " -> update solar " << x.first << endl;
  }

  for (auto x : g.waypoints){
    source_t key = identifier::make(identifier::waypoint, x.first);
    sf::Color col = sfcolor(players[socket.id].color);
    if (entity_selectors.count(key)) delete entity_selectors[key];
    entity_selectors[key] = new waypoint_selector(x.second, col);
    entity_selectors[key] -> seen = true;
    cout << " -> update waypoint " << x.first << endl;
  }

  // remove entities as server specifies
  for (auto x : g.remove_entities){
    if (entity_selectors.count(x)){
      cout << " -> remove entity " << x << endl;
      delete entity_selectors[x];
      entity_selectors.erase(x);
    }
  }

  // keep ships belonging to remaining fleets and solars
  hm_t<idtype, ship> ship_buf = ships;
  ships.clear();
  for (auto x : g.ships) ship_buf[x.first] = x.second;

  for (auto x : entity_selectors){
    if (x.second -> isa(identifier::fleet) || x.second -> isa(identifier::solar)){
      for (auto i : x.second -> get_ships()){
	ships[i] = ship_buf[i];
      }
    }
  }

  cout << " -> resulting entities: " << endl;
  for (auto x : entity_selectors) cout << x.first << endl;

  // propagate remaining ships through waypoints
  // fleets are sources
  set<waypoint_selector*> buf;
  for (auto x : entity_selectors){
    if (x.second -> owned && x.second -> isa(identifier::fleet)){
      fleet_selector *fs = (fleet_selector*)x.second;
      if (fs -> is_idle()){
	source_t wid = identifier::make(identifier::waypoint, identifier::get_string_id(fs -> com.target));
	if (entity_selectors.count(wid)){
	  waypoint_selector *wp = (waypoint_selector*)entity_selectors[wid];
	  wp -> ships += fs -> ships;
	  buf.insert(wp);
	}
      }else if (entity_selectors.count(fs -> com.target)){
	command c = fs -> com;
	point from = fs -> position;
	point to = entity_selectors[fs -> com.target] -> get_position();
	// assure we don't assign ships which have been killed
	c.ships = c.ships & fs -> ships;
	cout << " -> adding fleet fommand from " << c.source << " to " << c.target << " with " << c.ships.size() << " ships." << endl;

	add_command(c, from, to, false);
	if (entity_selectors[c.target] -> isa(identifier::waypoint)){
	  buf.insert((waypoint_selector*)entity_selectors[c.target]);
	}
      }else{
	cout << "client side target miss: " << fs -> com.target << endl;
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
      if (entity_selectors.count(c.target)){
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
	  point from = s -> get_position();
	  point to = t -> get_position();
	  c.ships = c.ships & s -> ships;
	  add_command(c, from, to, false);
	}

	if (t -> isa(identifier::waypoint)){
	  q.push((waypoint_selector*)t);
	}
      }else{
	cout << "client side target miss: " << c.target << endl;
      }
    }
  }

  // check ownerhsip for solar choices
  for (auto &x : solar_choices){
    if (!(entity_selectors.count(x.first) && entity_selectors[x.first] -> owned)) solar_choices.erase(x.first);
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
  if (command_exists(c)) {
    cout << "add_command: from " << c.source << " to " << c.target << " action " << c.action << ": already exists!" << endl;
    return;
  }

  entity_selector *s = entity_selectors[c.source];
  entity_selector *t = entity_selectors[c.target];

  // check waypoint ancestors
  if (waypoint_ancestor_of(c.target, c.source)){
    cout << "add_command: circular waypoint graphs forbidden!" << endl;
    return;
  }

  idtype id = comid++;
  command_selector *cs = new command_selector(c, from, to);

  cout << "add command: " << id << " from " << c.source << " to " << c.target << endl;
  
  // add command to command selectors
  command_selectors[id] = cs;

  // add command selector key to list of the source entity's children
  s -> commands.insert(id);

  // add ships to command
  set<idtype> ready_ships = get_ready_ships(c.source);
  cout << "ready ships: " << ready_ships.size() << endl;
  if (fill_ships) cs -> ships = ready_ships;
  // s -> allocated_ships += cs -> ships;

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
  // s -> allocated_ships -= a;

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
    // s -> allocated_ships -= cships;

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
    x.second -> seen = false;
  }

  for (auto x : command_selectors){
    delete x.second;
  }

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

void st3::client::game::command2entity(source_t key, string act, list<source_t> selected_entities){
  if (!entity_selectors.count(key)){
    cout << "command2entity: invalid key: " << key << endl;
    exit(-1);
  }

  command c;
  point from, to;
  entity_selector *s = entity_selectors[key];
  c.target = key;
  c.action = act;
  to = s -> get_position();

  cout << "command2entity: " << key << endl;

  for (auto x : selected_entities){
    if (entity_selectors.count(x) && x != key){
      cout << "command2entity: adding " << x << endl;
      c.source = x;
      from = entity_selectors[x] -> get_position();
      add_command(c, from, to);
    }
  }
}

set<source_t> st3::client::game::entities_at(point p){
  float d;
  set<source_t> keys;

  cout << "entities_at:" << endl;

  // find entities at p
  for (auto x : entity_selectors){
    cout << "checking entity: " << x.first << endl;
    if (x.second -> contains_point(p, d)) keys.insert(x.first);
  }

  return keys;
}

source_t st3::client::game::entity_at(point p, int *q){
  float d;
  set<source_t> keys = entities_at(p);
  source_t key = "";
  int qmin = selector_queue;

  cout << "entity_at:" << endl;

  // find closest entity to p
  for (auto x : keys){
    cout << "checking entity: " << x << endl;
    entity_selector *e = entity_selectors[x];
    if (e -> queue_level < qmin){
      qmin = e -> queue_level;
      key = x;
      cout << "entity_at: k = " << key << ", qmin = " << e -> queue_level << endl;
    }
  }

  cout << "entity at: " << p.x << "," << p.y << ": " << key << endl;

  if (q) *q = qmin;

  return key;
}


idtype st3::client::game::command_at(point p, int *q){
  int qmin = selector_queue;
  float d;
  idtype key = -1;

  // find closest entity to p
  for (auto x : command_selectors){
    if (x.second -> contains_point(p, d) && x.second -> queue_level < qmin){
      qmin = x.second -> queue_level;
      key = x.first;
    }
  }

  cout << "command at: " << p.x << "," << p.y << ": " << key << endl;

  if (q) *q = qmin;

  return key;
}

bool st3::client::game::select_at(point p){
  cout << "select at: " << p.x << "," << p.y << endl;
  int qent, qcom;
  source_t key = entity_at(p, &qent);
  idtype cid = command_at(p, &qcom);

  if (qcom < qent) return select_command(cid);

  auto it = entity_selectors.find(key);
 
  if (!add2selection()) deselect_all();

  if (it != entity_selectors.end() && it -> second -> owned){
    it -> second -> selected = !(it -> second -> selected);
    it -> second -> queue_level = selector_queue++;

    // debug printouts
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

bool st3::client::game::select_command(idtype key){
  auto it = command_selectors.find(key);
 
  if (!add2selection()) deselect_all();

  if (it != command_selectors.end()){
    it -> second -> selected = !(it -> second -> selected);
    it -> second -> queue_level = selector_queue++;
    
    cout << "command found: " << it -> first << " -> " << it -> second -> selected << endl;

    // setup command gui
    hm_t<idtype, ship> ready_ships;
    
    for (auto x : get_ready_ships(it -> second -> source)){
      ready_ships[x] = ships[x];
    }
    
    for (auto x : it -> second -> ships){
      ready_ships[x] = ships[x];
    }

    comgui = new command_gui(it -> first, 
			     &window, 
			     ready_ships,
			     it -> second -> ships,
			     view_window.getSize(),
			     sfcolor(players[socket.id].color),
			     "source: " + it -> second -> source 
			     + ", target: " + it -> second -> target 
			     + ", action: " + it -> second -> action);

    return true;
  }

  return false;
}

int game::count_selected(){
  int sum = 0;
  for (auto x : entity_selectors) sum += x.second -> selected;
  return sum;
}

// ids of non-allocated ships for entity selector
set<idtype> game::get_ready_ships(source_t id){

  if (!entity_selectors.count(id)) {
    cout << "get ready ships: entity selector " << id << " not found!" << endl;
    exit(-1);
  }

  entity_selector *e = entity_selectors[id];
  set<idtype> s = e -> get_ships();
  for (auto c : e -> commands){
    for (auto x : command_selectors[c] -> ships){
      s.erase(x);
    }
  }

  return s;
}

list<source_t> game::selected_solars(){
  list<source_t> res;
  for (auto &x : entity_selectors){
    if (x.second -> isa(identifier::solar) && x.second -> selected){
      res.push_back(x.first);
    }
  }
  return res;
}

list<source_t> game::selected_entities(){
  list<source_t> res;
  for (auto &x : entity_selectors){
    if (x.second -> selected) res.push_back(x.first);
  }
  return res;
}

void game::run_solar_gui(source_t key){
  solar::solar sol = *((solar_selector*)entity_selectors[key]);
  if (sol.owner < 0) {
    cout << "run solar gui: no owner!" << endl;
    exit(-1);
  }

  interface::desktop -> reset_qw(interface::solar_query::main_window::Create(identifier::get_id(key), sol));
}

// return true to signal choice step done
int st3::client::game::choice_event(sf::Event e){
  point p;
  list<source_t> ss;

  window.setView(view_game);
  if (targui && targui -> handle_event(e)){
    if (targui -> done){
      if (targui -> selected_option.key != "cancel"){
	source_t k = targui -> selected_option.key;
	if (k == "add_waypoint"){
	  k = add_waypoint(targui -> position);
	}

	command2entity(k, targui -> selected_option.option, targui -> selected_entities);
      }
      delete targui;
      targui = 0;
    }
    return query_query;
  }

  window.setView(view_window);
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

      ws -> ships += added;

      if (!removed.empty()){
	recursive_waypoint_deallocate(cs -> target, removed);
      }
    }

    // set command selector's ships
    cs -> ships = comgui -> allocated;
    
    return query_query;
  }

  window.setView(view_game);
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
      }else{
	select_at(p);
      }
    } else if (e.mouseButton.button == sf::Mouse::Right && count_selected()){
      // target_at(p);
      // set up targui
      set<source_t> keys = entities_at(p);
      list<target_gui::option_t> options;
      
      // check if a colonizer is available
      bool has_colonizer = false;
      for (auto x : entity_selectors){
	if (x.second -> selected){
	  for (auto i : x.second -> get_ships()){
	    has_colonizer |= ships[i].ship_class == "colonizer";
	  }
	}
      }

      if (keys.empty()){
	options.push_back(target_gui::option_add_waypoint);
      }else{
	for (auto x : keys){
	  entity_selector *e = entity_selectors[x];
	  if (e -> isa(identifier::waypoint)){
	    options.push_back(target_gui::option_t(x, command::action_waypoint));
	  }else if (e -> isa(identifier::fleet)){
	    if (e -> owned){
	      options.push_back(target_gui::option_t(x, command::action_follow));
	      options.push_back(target_gui::option_t(x, command::action_join));
	    }else{
	      options.push_back(target_gui::option_t(x, command::action_follow));
	      options.push_back(target_gui::option_t(x, command::action_attack));
	    }
	  }else if (e -> isa(identifier::solar)){
	    if (e -> owned){
	      // owned solar
	      options.push_back(target_gui::option_t(x, command::action_land));
	    }else{
	      // non-owned solar
	      solar_selector *ss = (solar_selector*)e;
	      options.push_back(target_gui::option_t(x, command::action_attack));
	      if (ss -> owner == -1 
		  && !ss -> has_defense()
		  && has_colonizer){
		// undefended neutral solar
		options.push_back(target_gui::option_t(x, command::action_colonize));
	      }
	    }
	  }
	}
      }

      options.push_back(target_gui::option_cancel);
      targui = new target_gui(p, options, selected_entities(), &window);
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
      if (comgui || targui){
	clear_guis();
      }else{
	cout << "choice_event: proceed" << endl;
	return query_accepted;
      }
      break;
    case sf::Keyboard::Return:
      ss = selected_solars();
      if (ss.size() == 1) run_solar_gui(ss.front());
      break;
    case sf::Keyboard::Delete:
      for (auto i : list<pair<idtype, command_selector*> >(command_selectors.begin(), command_selectors.end())){
	if (i.second -> selected){
	  remove_command(i.first);
	}
      }
      break;
    case sf::Keyboard::O:
      view_game.zoom(1.2);
      break;
    case sf::Keyboard::I:
      view_game.zoom(1 / 1.2);
      break;
    }
    break;
  };

  return query_query;
}

void st3::client::game::controls(){
  static point vel(0,0);
  if (!window.hasFocus()) {
    vel = point(0,0);
    return;
  }

  float s = view_game.getSize().x / settings.width;

  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)){
    vel.x -= 15 * s;
  } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)){
    vel.x += 15 * s;
  } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)){
    vel.y -= 15 * s;
  } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)){
    vel.y += 15 * s;
  }

  vel = utility::scale_point(vel, 0.8);
  
  view_game.move(vel);
}

// ****************************************
// GRAPHICS
// ****************************************
using namespace graphics;

int game::window_loop(int &done, function<int(sf::Event)> event_handler, function<int(void)> body){
  sf::Clock clock;

  while (!done){
    auto delta = clock.restart().asSeconds();

    sf::Event event;
    while (window.pollEvent(event)){
      done |= event_handler(event);
    }
    
    if (!window.isOpen()) done |= query_aborted;

    done |= body();

    // update controls
    controls();

    // draw universe and game objects
    draw_window();

    window.setView(view_window);

    // update sfgui
    interface::desktop -> Update(delta);

    // draw sfgui
    sfgui -> Display(window);

    // display on screen
    window.display();
  }

  return done;
}

void game::draw_window(){
  window.clear();

  // draw main interface
  window.setView(view_game);
  point ul = ul_corner(window);
  draw_universe();
  draw_interface_components();

  // draw targui
  window.setView(view_game);
  if (targui) targui -> draw();

  // draw command gui
  if (comgui) {
    window.setView(view_window);
    comgui -> draw();
  }else{
    window.setView(view_window);

    // draw text
    sf::Text text;
    text.setFont(default_font); 
    text.setCharacterSize(24);
    text.setColor(sf::Color(200,200,200));
    text.setString(message);
    text.setPosition(point(10,10));
    window.draw(text);

    // draw minimap bounds
    sf::FloatRect fr = view_minimap.getViewport();
    sf::RectangleShape r;
    r.setPosition(fr.left * view_window.getSize().x, fr.top * view_window.getSize().y);
    r.setSize(point(fr.width * view_window.getSize().x, fr.height * view_window.getSize().y));
    r.setOutlineColor(sf::Color(255,255,255));
    r.setFillColor(sf::Color(0,0,25,100));
    r.setOutlineThickness(1);

    window.draw(r);

    // draw minimap contents
    window.setView(view_minimap);
    draw_universe();
    r.setOutlineColor(sf::Color(0, 255, 0));
    r.setPosition(ul);
    r.setSize(view_game.getSize());
    window.draw(r);
  }

  // reset game view
  window.setView(view_game);
}

void game::draw_interface_components(){
  window.setView(view_game);

  // draw commands
  for (auto x : command_selectors) x.second -> draw(window);

  if (area_select_active && srect.width && srect.height){
    // draw selection rect
    sf::RectangleShape r = build_rect(srect);
    r.setFillColor(sf::Color(250,250,250,50));
    r.setOutlineColor(sf::Color(80, 120, 240, 200));
    r.setOutlineThickness(1);
    window.draw(r);
  }

}

void st3::client::game::draw_universe(){

  for (auto star : fixed_stars) star.draw (window);

  // draw source/target entities
  for (auto x : entity_selectors) x.second -> draw(window);

  // draw ships in fleets
  for (auto x : ships){
    source_t key = identifier::make(identifier::fleet, x.second.fleet_id);
    if (entity_selectors.count(key)){
      draw_ship(window, x.second, entity_selectors[key] -> get_color());
    }
  }
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
