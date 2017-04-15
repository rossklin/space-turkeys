#include <iostream>
#include <thread>
#include <queue>
#include <type_traits>

#include <SFML/Graphics.hpp>

#include "client_game.h"
#include "graphics.h"
#include "com_client.h"
#include "protocol.h"
#include "serialization.h"
#include "utility.h"
#include "command_gui.h"
#include "target_gui.h"

using namespace std;
using namespace st3;
using namespace client;
using namespace graphics;

// local utility functions
sf::FloatRect fixrect(sf::FloatRect r);
bool add2selection();
bool ctrlsel();

waypoint_selector::ptr to_wps(entity_selector::ptr p){
  return utility::guaranteed_cast<waypoint_selector, entity_selector>(p);
}

// ****************************************
// GAME STEPS
// ****************************************

game::game(){
  comgui = 0;
  targui = 0;
  selector_queue = 1;
  chosen_quit = false;

  default_event_handler = [this](sf::Event e) -> int {
    if (e.type == sf::Event::Closed) {
      window.close();
      return query_aborted;
    }else{
      interface::desktop -> HandleEvent(e);
      return 0;
    }
  };

  default_body = [this] () -> int {
    if (window.isOpen()) {return 0;} else {return query_aborted;}
  };
}

entity_selector::ptr game::get_entity(combid i){
  if (entity.count(i)){
    return entity[i];
  }else{
    cout << "client::game::get_entity: invalid id: " << i << endl;
    exit(-1);
  }
}

template<typename T>
typename specific_selector<T>::ptr game::get_specific(combid i){
  if (entity.count(i)){
    return utility::guaranteed_cast<specific_selector<T>, entity_selector>(entity[i]);
  }else{
    cout << "client::game::get_specific: invalid id: " << i << endl;
    exit(-1);
  }
}

void game::clear_guis(){
  if (comgui) delete comgui;
  comgui = 0;
  if (targui) delete targui;
  targui = 0;
}

void game::run(){  
  
  bool proceed = true;
  bool first = true;
  area_select_active = false;
  view_game = sf::View(sf::FloatRect(0, 0, settings.width, settings.height));
  view_minimap = view_game;
  view_minimap.setViewport(sf::FloatRect(0.01, 0.71, 0.28, 0.28));
  view_window = window.getDefaultView();
  self_id = socket -> id;

  // construct interface
  window.setView(view_window);
  interface::desktop = new interface::main_interface(window.getSize(), players[self_id].research_level);
  col = sfcolor(players[self_id].color);
  
  // game loop
  while (true){
    clear_guis();

    if (!pre_step()) break;

    if (first){
      // initialize position at home solar
      first = false;
      for (auto i : entity){
	if (i.second -> isa(solar::class_id) && i.second -> owned){
	  view_game.setCenter(i.second -> get_position());
	  view_game.setSize(point(25 * settings.solar_maxrad, 25 * settings.solar_maxrad));
	  break;
	}
      }
    }

    if (!choice_step()) break;

    if (!simulation_step()) break;
  }

  if (!chosen_quit){
    string server_says;
    socket -> data >> server_says;
    popup_message("GAME OVER", server_says);
  }
  
  delete interface::desktop;
  interface::desktop = 0;
}

// ****************************************
// PRE_STEP
// ****************************************

bool game::pre_step(){
  int done = 0;
  sf::Packet pq;
  data_frame data;

  message = "loading game data...";
  pq << protocol::game_round;

  thread t(query, 
	   socket, 
	   ref(pq),
	   ref(done));

  window_loop(done, default_event_handler, default_body);

  cout << "pre_step: waiting for com thread to finish..." << endl;
  t.join();

  if (done & (query_game_complete | query_aborted)){
    cout << "pre_step: finished/aborted" << endl;
    return false;
  }

  if (!deserialize(data, socket -> data, col, self_id)){
    cout << "pre_step: failed to deserialize game_data" << endl;
    return false;
  }

  reload_data(data);

  return true;
}

// ****************************************
// CHOICE STEP
// ****************************************

bool game::choice_step(){
  int done = 0;
  sf::Packet pq, pr;
  interface::desktop -> done = false;
  interface::desktop -> accept = false;

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

  auto body = [this] () -> int {
    if (interface::desktop -> done) {
      return interface::desktop -> accept ? query_accepted : query_game_complete;
    }
    return default_body();
  };

  window_loop(done, event_handler, body);

  if (done & (query_game_complete | query_aborted)){
    cout << "choice_step: finishded" << endl;
    pq.clear();
    pq << protocol::leave;
    while (!socket -> send_packet(pq)) sf::sleep(sf::milliseconds(100));
    return false;
  }

  // SEND THE CHOICE TO SERVER

  message = "sending choice to server...";

  // add commands to choice
  choice::choice c = build_choice(interface::desktop -> response);
  done = 0;
  pq << protocol::choice;
  pq << c;

  thread t(query, 
	   socket, 
	   ref(pq),
	   ref(done));

  cout << "choice step: sending" << endl;

  window_loop(done, default_event_handler, default_body);

  if (done & query_game_complete){
    cout << "choice send: server says game finished!" << endl;
    return false;
  }

  clear_guis();
  deselect_all();

  cout << "choice step: waiting for query thread" << endl;
  t.join();
  cout << "choice step: complete" << endl;

  return true;
}

// ****************************************
// SIMULATION STEP
// ****************************************

bool game::simulation_step(){
  int done = 0;
  bool playing = true;
  int idx = -1;
  int loaded = 0;
  vector<data_frame> g(settings.frames_per_round);

  cout << "simluation: starting data loader" << endl;
  
  thread t(load_frames, socket, ref(g), ref(loaded), ref(done), col);

  auto event_handler = [this, &playing] (sf::Event e) -> int {
    switch (e.type){
    case sf::Event::Closed:
      window.close();
      return query_aborted;
    case sf::Event::KeyPressed:
      if (e.key.code == sf::Keyboard::Escape){
	return query_aborted;
      }else if (e.key.code == sf::Keyboard::Space){
        playing = !playing;
      }
    }
    return 0;
  };

  auto body = [&,this] () -> int {
    if (!window.isOpen()){
      cout << "simulation: aborted by window close" << endl;
      return query_aborted;
    }
    
    if (idx == settings.frames_per_round - 1) {
      cout << "simulation: all loaded" << endl;
      return query_accepted;
    }

    // draw load progress
    window.setView(view_window);

    auto colored_rect = [] (sf::Color c, float r) -> sf::RectangleShape{
      float w = interface::desktop_dims.x - 20;
      auto rect = graphics::build_rect(sf::FloatRect(10, interface::top_height + 10, r * w, 20));
      c.a = 150;
      rect.setFillColor(c);
      rect.setOutlineColor(sf::Color::White);
      rect.setOutlineThickness(1);
      return rect;
    };

    window.draw(colored_rect(sf::Color::Red, 1));
    window.draw(colored_rect(sf::Color::Blue, loaded / (float) settings.frames_per_round));
    window.draw(colored_rect(sf::Color::Green, idx / (float) settings.frames_per_round));

    message = "evolution: " + to_string((100 * idx) / settings.frames_per_round) + " %" + (playing ? "" : "(paused)");

    playing &= idx < loaded - 1;

    cout << "simulation: frame " << idx << endl;

    if (playing){
      cout << "simulation: step" << endl;
      idx++;
      reload_data(g[idx]);
    }
    return 0;
  };

  cout << "simlulation: starting loop" << endl;

  window_loop(done, event_handler, body);

  if (done & (query_game_complete | query_aborted)){
    cout << "simulation step: game aborted" << endl;
    t.join();
    return false;
  }

  cout << "simulation: waiting for thread join" << endl;
  t.join();

  cout << "simulation: finished." << endl;
  return true;
}

// ****************************************
// DATA HANDLING
// ****************************************

combid game::add_waypoint(point p){
  waypoint buf;
  buf.position = p;
  waypoint_selector::ptr w = waypoint_selector::create(buf, col, true);
  entity[w -> id] = w;

  cout << "added waypoint " << w -> id << endl;

  return w -> id;
}

command game::build_command(idtype key){
  if (!command_selectors.count(key)){
    cout << "build_command: not found: " << key << endl;
    exit(-1);
  }

  return (command)*command_selectors[key];
}

choice::choice game::build_choice(choice::choice c){
  cout << "build choice:" << endl;
  for (auto x : entity){
    if (x.second -> isa(waypoint::class_id)){
      waypoint_selector::ptr ws = to_wps(x.second);
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
      c.waypoints[x.first] = w;
    }else{
      for (auto y : x.second -> commands){
	c.commands[x.first].push_back(build_command(y));
	cout << "adding command " << y << " with key " << x.first << ", source " << c.commands[x.first].back().source << " and target " << c.commands[x.first].back().target << endl;
      }
    }
  }

  return c;
}

list<idtype> game::incident_commands(combid key){
  list<idtype> res;

  for (auto x : command_selectors){
    if (x.second -> target == key){
      res.push_back(x.first);
    }
  }

  return res;
}

// written by Johan Mattsson
void game::add_fixed_stars (point position, float vision) {
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

void game::reload_data(data_frame &g){
  cout << "reload data:" << endl;
  cout << " -> initial entities: " << endl;
  for (auto x : entity) cout << x.first << endl;

  // make selectors 'not seen' and clear commands
  clear_selectors();
  
  players = g.players;
  settings = g.settings;

  // update entities: fleets, solars and waypoints
  for (auto x : g.entity){
    combid key = x.first;
    entity[x.first] = x.second;
    x.second -> seen = true;
    if (x.second -> owner == self_id) add_fixed_stars (x.second -> position, x.second -> vision());
  }

  // remove entities as server specifies
  for (auto x : g.remove_entities){
    if (entity.count(x)){
      cout << " -> remove entity " << x << endl;
      entity.erase(x);
    }
  }

  // propagate remaining ships through waypoints
  // fleets are sources
  set<waypoint_selector::ptr> buf;
  for (auto x : entity){
    if (x.second -> owned && x.second -> isa(fleet::class_id)){
      fleet_selector::ptr fs = get_specific<fleet>(x.first);

      if (fs -> is_idle()){
	combid wid = fs -> com.target;
	if (entity.count(wid)){
	  waypoint_selector::ptr wp = get_specific<waypoint>(wid);
	  wp -> ships += fs -> ships;
	  buf.insert(wp);
	}
      }else if (entity.count(fs -> com.target)){
	command c = fs -> com;
	point from = fs -> position;
	point to = entity[fs -> com.target] -> get_position();
	// assure we don't assign ships which have been killed
	c.ships = c.ships & fs -> ships;
	cout << " -> adding fleet fommand from " << c.source << " to " << c.target << " with " << c.ships.size() << " ships." << endl;

	add_command(c, from, to, false);
	if (entity[c.target] -> isa(waypoint::class_id)){
	  buf.insert(get_specific<waypoint>(c.target));
	}
      }else{
	cout << "client side target miss: " << fs -> com.target << endl;
      }
    }
  }

  // move to queue for better propagation order
  queue<waypoint_selector::ptr> q;
  for (auto x : buf) q.push(x);

  while(!q.empty()){
    waypoint_selector::ptr s = q.front();
    q.pop();
    for (auto c : s -> pending_commands){
      if (entity.count(c.target)){
	entity_selector::ptr t = entity[c.target];
	idtype cid = command_id(c);
	if (cid > -1){
	  // command selector already added
	  command_selector::ptr cs = command_selectors[cid];
	
	  // add new ships to command selector
	  cs -> ships += c.ships & s -> ships;
	  cout << "filling cs " << cid << " with " << cs -> ships.size() << " ships" << endl;
	
	  // add new ships to target waypoint
	  if (t -> isa(waypoint::class_id)){
	    waypoint_selector::ptr ws = to_wps(t);
	    ws -> ships += cs -> ships;
	  }
	}else{
	  point from = s -> get_position();
	  point to = t -> get_position();
	  c.ships = c.ships & s -> ships;
	  add_command(c, from, to, false);
	}

	if (t -> isa(waypoint::class_id)){
	  waypoint_selector::ptr ws = to_wps(t);
	  q.push(ws);
	}
      }else{
	cout << "client side target miss: " << c.target << endl;
      }
    }
  }
}

// ****************************************
// COMMAND MANIPULATION
// ****************************************

bool game::command_exists(command c){
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

void game::add_command(command c, point from, point to, bool fill_ships){
  if (command_exists(c)) {
    cout << "add_command: from " << c.source << " to " << c.target << " action " << c.action << ": already exists!" << endl;
    return;
  }

  entity_selector::ptr s = entity[c.source];
  entity_selector::ptr t = entity[c.target];

  // check waypoint ancestors
  if (waypoint_ancestor_of(c.target, c.source)){
    cout << "add_command: circular waypoint graphs forbidden!" << endl;
    return;
  }

  idtype id = comid++;
  command_selector::ptr cs = command_selector::create(c, from, to);

  cout << "add command: " << id << " from " << c.source << " to " << c.target << endl;
  
  // add command to command selectors
  command_selectors[id] = cs;

  // add command selector key to list of the source entity's children
  s -> commands.insert(id);

  // add ships to command
  set<combid> ready_ships = get_ready_ships(c.source);
  cout << "ready ships: " << ready_ships.size() << endl;
  if (fill_ships) cs -> ships = ready_ships;
  // s -> allocated_ships += cs -> ships;

  // add ships to waypoint
  if (t -> isa(waypoint::class_id)){
    waypoint_selector::ptr ws = to_wps(t);
    ws -> ships += cs -> ships;
  }

  cout << "added command: " << id << endl;
}

void game::recursive_waypoint_deallocate(combid wid, set<combid> a){
  entity_selector::ptr es = entity[wid];
  if (!es -> isa(waypoint::class_id)) return;
  waypoint_selector::ptr s = to_wps(es);

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
    entity.erase(wid);
    return;
  }

  if (a.empty()) return;

  s -> ships -= a;
  // s -> allocated_ships -= a;

  cout << "removing " << a.size() << " ships from waypoint " << wid << endl;
  
  for (auto c : s -> commands){
    command_selector::ptr cs = command_selectors[c];
    
    cout << " .. removing ships from command " << c << endl;
    // ships to remove from this command
    set<combid> delta = cs -> ships & a;
    cs -> ships -= delta;

    cout << " .. recursive call RWD on " << cs -> target << endl;
    recursive_waypoint_deallocate(cs -> target, delta);
  }
}

bool game::waypoint_ancestor_of(combid ancestor, combid child){
  // only waypoints can have this relationship
  if (identifier::get_type(ancestor).compare(waypoint::class_id) || identifier::get_type(child).compare(waypoint::class_id)){
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
void game::remove_command(idtype key){
  cout << "remove command: " << key << endl;

  if (command_selectors.count(key)){
    command_selector::ptr cs = command_selectors[key];
    entity_selector::ptr s = entity[cs -> source];
    set<combid> cships = cs -> ships;
    combid target_key = cs -> target;

    cout << " .. deleting; source = " << cs -> source << endl;
    // remove this command's selector
    command_selectors.erase(key);

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

void game::clear_selectors(){
  comid = 0;

  for (auto x : entity){
    x.second -> seen = false;
  }

  command_selectors.clear();
}

void game::deselect_all(){
  for (auto x : entity) x.second -> selected = false;
  for (auto x : command_selectors) x.second -> selected = false;
}

// ****************************************
// EVENT HANDLING
// ****************************************

void game::area_select(){
  sf::FloatRect rect = fixrect(srect);

  if (!add2selection()) deselect_all();

  cout << "area select" << endl;
  
  for (auto x : entity){
    x.second -> selected = x.second -> owned && x.second -> area_selectable && x.second -> inside_rect(rect);
  }
}

void game::command2entity(combid key, string act, list<combid> selected_entities){
  if (!entity.count(key)){
    cout << "command2entity: invalid key: " << key << endl;
    exit(-1);
  }

  command c;
  point from, to;
  entity_selector::ptr s = entity[key];
  c.target = key;
  c.action = act;
  to = s -> get_position();

  cout << "command2entity: " << key << endl;

  for (auto x : selected_entities){
    if (entity.count(x) && x != key){
      cout << "command2entity: adding " << x << endl;
      c.source = x;
      from = entity[x] -> get_position();
      add_command(c, from, to);
    }
  }
}

set<combid> game::entities_at(point p){
  float d;
  set<combid> keys;

  cout << "entities_at:" << endl;

  // find entities at p
  for (auto x : entity){
    cout << "checking entity: " << x.first << endl;
    if (x.second -> contains_point(p, d)) keys.insert(x.first);
  }

  return keys;
}

combid game::entity_at(point p, int *q){
  float d;
  set<combid> keys = entities_at(p);
  combid key = "";
  int qmin = selector_queue;

  cout << "entity_at:" << endl;

  // find closest entity to p
  for (auto x : keys){
    cout << "checking entity: " << x << endl;
    entity_selector::ptr e = entity[x];
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


idtype game::command_at(point p, int *q){
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

bool game::select_at(point p){
  cout << "select at: " << p.x << "," << p.y << endl;
  int qent, qcom;
  combid key = entity_at(p, &qent);
  idtype cid = command_at(p, &qcom);

  if (qcom < qent) return select_command(cid);

  auto it = entity.find(key);
 
  if (!add2selection()) deselect_all();

  if (it != entity.end() && it -> second -> owned){
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

bool game::select_command(idtype key){
  auto it = command_selectors.find(key);
 
  if (!add2selection()) deselect_all();

  if (it != command_selectors.end()){
    it -> second -> selected = !(it -> second -> selected);
    it -> second -> queue_level = selector_queue++;
    
    cout << "command found: " << it -> first << " -> " << it -> second -> selected << endl;

    // setup command gui
    hm_t<combid, ship> ready_ships;

    // TODO: this doesn't work for solars, since they contain ships
    // which are not registered in 'entity'. Probably rewrite solar to
    // store only ship ids.
    for (auto x : get_ready_ships(it -> second -> source)){
      ready_ships[x] = (ship)*get_specific<ship>(x);
    }
    
    for (auto x : it -> second -> ships){
      ready_ships[x] = (ship)*get_specific<ship>(x);
    }

    comgui = new command_gui(it -> first, 
			     &window, 
			     ready_ships,
			     it -> second -> ships,
			     view_window.getSize(),
			     col,
			     "source: " + it -> second -> source 
			     + ", target: " + it -> second -> target 
			     + ", action: " + it -> second -> action);

    return true;
  }

  return false;
}

int game::count_selected(){
  int sum = 0;
  for (auto x : entity) sum += x.second -> selected;
  return sum;
}

// ids of non-allocated ships for entity selector
set<combid> game::get_ready_ships(combid id){

  if (!entity.count(id)) {
    cout << "get ready ships: entity selector " << id << " not found!" << endl;
    exit(-1);
  }

  entity_selector::ptr e = entity[id];
  set<combid> s = e -> get_ships();
  for (auto c : e -> commands){
    for (auto x : command_selectors[c] -> ships){
      s.erase(x);
    }
  }

  return s;
}

list<combid> game::selected_solars(){
  list<combid> res;
  for (auto &x : entity){
    if (x.second -> isa(solar::class_id) && x.second -> selected){
      res.push_back(x.first);
    }
  }
  return res;
}

list<combid> game::selected_entities(){
  list<combid> res;
  for (auto &x : entity){
    if (x.second -> selected) res.push_back(x.first);
  }
  return res;
}

void game::run_solar_gui(combid key){
  interface::desktop -> reset_qw(interface::main_window::Create(get_specific<solar>(key)));
}

// return true to signal choice step done
int game::choice_event(sf::Event e){
  point p;
  list<combid> ss;

  if (interface::desktop -> query_window) {
    if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Escape){
      interface::desktop -> clear_qw();
    }
    return 0;
  }

  window.setView(view_game);
  if (targui && targui -> handle_event(e)){
    if (targui -> done){
      if (targui -> selected_option.key != "cancel"){
	combid k = targui -> selected_option.key;
	if (k == "add_waypoint"){
	  k = add_waypoint(targui -> position);
	}

	command2entity(k, targui -> selected_option.option, targui -> selected_entities);
      }
      delete targui;
      targui = 0;
    }
    return 0;
  }

  window.setView(view_window);
  if (comgui && comgui -> handle_event(e)){
    // handle comgui effects

    command_selector::ptr cs = command_selectors[comgui -> comid];
    entity_selector::ptr s = entity[cs -> source];
    entity_selector::ptr t = entity[cs -> target];

    // check if target is a waypoint
    if (t -> isa(waypoint::class_id)){
      waypoint_selector::ptr ws = to_wps(t);

      // compute added or removed ships
      set<combid> removed = cs -> ships - comgui -> allocated;
      set<combid> added = comgui -> allocated - cs -> ships;

      ws -> ships += added;

      if (!removed.empty()){
	recursive_waypoint_deallocate(cs -> target, removed);
      }
    }

    // set command selector's ships
    cs -> ships = comgui -> allocated;
    
    return 0;
  }

  sf::Vector2i mpos;
  sf::FloatRect minirect;
  point delta;
  point target;
	  
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
    mpos = sf::Vector2i(e.mouseButton.x, e.mouseButton.y);
    p = window.mapPixelToCoords(mpos);
    
    if (e.mouseButton.button == sf::Mouse::Left){
      if (abs(srect.width) > 5 || abs(srect.height) > 5){
	area_select();
      }else{
	// check if on minimap
	minirect = minimap_rect();
	if (minirect.contains(mpos.x, mpos.y)){
	  delta = point(mpos.x - minirect.left, mpos.y - minirect.top);
	  target = point(delta.x / minirect.width * settings.width, delta.y / minirect.height * settings.height);

	  cout << "minimap clicked, setting center: " << target.x << "x" << target.y << endl;
	  
	  view_game.setCenter(target);
	}else{
	  select_at(p);
	}
      }
    } else if (e.mouseButton.button == sf::Mouse::Right && count_selected()){
      // set up targui
      auto keys_targeted = entities_at(p);
      auto keys_selected = selected_entities();
      list<target_gui::option_t> options;
      set<string> possible_actions = fleet::all_base_actions();

      // add possible actions from available fleet interactions
      for (auto k : keys_selected){
	entity_selector::ptr e = entity[k];
	if (e -> isa(fleet::class_id)){
	  fleet_selector::ptr f = utility::guaranteed_cast<fleet_selector, entity_selector>(e);
	  possible_actions += f -> interactions;
	}
      }

      // check if actions are allowed per target
      auto atab = fleet::action_condition_table();
      for (auto a : possible_actions){
	auto condition = atab[a].owned_by(self_id);
	for (auto k : keys_targeted){
	  if (interaction::valid(condition, entity[k])){
	    options.push_back(target_gui::option_t(k, a));
	  }
	}
      }

      // default options
      options.push_back(target_gui::option_add_waypoint);
      options.push_back(target_gui::option_cancel);

      targui = new target_gui(p, options, keys_selected, &window);
    }

    // clear selection rect
    area_select_active = false;
    srect = sf::FloatRect(0, 0, 0, 0);
    break;
  case sf::Event::MouseMoved:
    p = window.mapPixelToCoords(sf::Vector2i(e.mouseMove.x, e.mouseMove.y));
    
    if (area_select_active){
      // update area selection
      srect.width = p.x - srect.left;
      srect.height = p.y - srect.top;
      cout << "update srect size at " << p.x << "x" << p.y << " to " << srect.width << "x" << srect.height << endl;
    }else{
      // update hover info
      auto keys = entities_at(p);
      string text = "Empty space";
      
      if (keys.size() == 1){
	combid k = *keys.begin();
	if (!entity.count(k)) {
	  cout << "update hover info: entity " << k << " not in table!" << endl;
	  exit(-1);
	}

	text = entity[k] -> hover_info();
      }else if (keys.size() > 1){
	text = "Multiple entities here!";
      }

      interface::desktop -> hover_label -> SetText(text);
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
      for (auto i = command_selectors.begin(); i != command_selectors.end(); i++){
	if (i -> second -> selected){
	  remove_command((i++) -> first);
	}
      }
      break;
    case sf::Keyboard::O:
      view_game.zoom(1.2);
      break;
    case sf::Keyboard::I:
      view_game.zoom(1 / 1.2);
      break;
    case sf::Keyboard::Escape:
      window.setView(view_window);
      if(popup_query("Really quit?")){
	chosen_quit = true;
	return query_aborted;
      }
    }
    break;
  };

  return 0;
}

void game::controls(){
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

void game::popup_message(string title, string message){
  int done = false;

  auto w = sfg::Window::Create();
  auto layout = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 10);
  auto text = sfg::Label::Create(message);
  auto baccept = sfg::Button::Create("OK");

  baccept -> GetSignal(sfg::Widget::OnLeftClick).Connect([&] () {
      done = true;
    });

  layout -> Pack(text);
  layout -> Pack(baccept);
  w -> SetTitle(title);
  w -> Add(layout);
  w -> SetPosition(sf::Vector2f(window.getSize().x / 2 - w -> GetRequisition().x / 2, window.getSize().y / 2 - w -> GetRequisition().y / 2));

  interface::desktop -> Add(w);

  auto event_handler = [this] (sf::Event e) -> int {
    if (e.type == sf::Event::KeyPressed){
      if (e.key.code == sf::Keyboard::Escape){
	return query_aborted;
      }else if (e.key.code == sf::Keyboard::Return){
	return query_accepted;
      }
    }
    return default_event_handler(e);
  };


  window_loop(done, event_handler, default_body);

  interface::desktop -> Remove(w);
  
  cout << "popup message: done" << endl;
}

bool game::popup_query(string v){  
  int done = false;
  bool accept = false;

  cout << "popup query: start" << endl;

  auto w = sfg::Window::Create();
  auto layout = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 10);
  auto blayout = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 10);
  auto header = sfg::Label::Create(v);
  auto baccept = sfg::Button::Create("OK");
  auto bcancel = sfg::Button::Create("Cancel");

  baccept -> GetSignal(sfg::Widget::OnLeftClick).Connect([&] () {
      accept = true;
      done = true;
    });

  bcancel -> GetSignal(sfg::Widget::OnLeftClick).Connect([&] () {
      done = true;
      accept = false;
    });

  blayout -> Pack(bcancel);
  blayout -> Pack(baccept);
  layout -> Pack(header);
  layout -> Pack(blayout);
  w -> Add(layout);
  interface::desktop -> Add(w);

  w -> SetPosition(sf::Vector2f(window.getSize().x / 2 - w -> GetRequisition().x / 2, window.getSize().y / 2 - w -> GetRequisition().y / 2));

  auto event_handler = [this, &accept] (sf::Event e) -> int {
    if (e.type == sf::Event::KeyPressed){
      if (e.key.code == sf::Keyboard::Escape){
	accept = false;
	return query_aborted;
      }else if (e.key.code == sf::Keyboard::Return){
	accept = true;
	return query_accepted;
      }
    }
    return default_event_handler(e);
  };

  window_loop(done, event_handler, default_body);

  interface::desktop -> Remove(w);
  
  cout << "popup: done: " << accept << endl;
  return accept;
}


void game::window_loop(int &done, function<int(sf::Event)> event_handler, function<int(void)> body){
  sf::Clock clock;
  float frame_time = 1/(float)20;

  while (!done){
    auto delta = clock.restart().asSeconds();

    sf::Event event;
    while (window.pollEvent(event)){
      done |= event_handler(event);
    }
    
    if (!window.isOpen()) done |= query_aborted;

    // update controls
    controls();

    // draw universe and game objects
    draw_window();

    // main content callback
    done |= body();

    window.setView(view_window);
    // todo: add a dummy draw here to ensure sfgui uses this view
    

    // update sfgui
    interface::desktop -> Update(delta);

    // draw sfgui
    sfgui -> Display(window);

    // display on screen
    window.display();

    sf::Time wait_for = sf::seconds(fmax(frame_time - clock.getElapsedTime().asSeconds(), 0));
    sf::sleep(wait_for);
  }

  cout << "window_loop: done: " << done << endl;
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
  }else if (interface::desktop -> query_window == 0){

    // draw minimap contents
    window.setView(view_minimap);
    draw_universe();

    // draw view rectangle
    sf::RectangleShape r;
    sf::Vector2f center = view_game.getCenter();
    sf::Vector2f size = view_game.getSize();
    r.setPosition(center.x - size.x / 2, center.y - size.y / 2);
    r.setSize(sf::Vector2f(size.x, size.y));
    r.setFillColor(sf::Color::Transparent);
    r.setOutlineColor(sf::Color::Green);
    r.setOutlineThickness(1);
    window.draw(r);
    
    window.setView(view_window);

    // draw text
    sf::Text text;
    text.setFont(default_font); 
    text.setCharacterSize(24);
    text.setColor(sf::Color(200,200,200));
    text.setString(message);
    text.setPosition(point(10, 0.2 * (interface::desktop_dims.y)));
    window.draw(text);

    // draw minimap bounds
    sf::FloatRect fr = minimap_rect();
    r.setPosition(fr.left, fr.top);
    r.setSize(sf::Vector2f(fr.width, fr.height));
    r.setOutlineColor(sf::Color(255,255,255));
    r.setFillColor(sf::Color(0,0,25,100));
    r.setOutlineThickness(1);
    window.draw(r);
  }
}

sf::FloatRect game::minimap_rect(){
  sf::FloatRect fr = view_minimap.getViewport();
  sf::FloatRect r;
  r.left = fr.left * view_window.getSize().x;
  r.top = fr.top * view_window.getSize().y;
  r.width = fr.width * view_window.getSize().x;
  r.height = fr.height * view_window.getSize().y;
  return r;
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

void game::draw_universe(){
  for (auto star : fixed_stars) star.draw(window);
  for (auto x : entity) x.second -> draw(window);
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
