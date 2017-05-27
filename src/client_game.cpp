#include <iostream>
#include <thread>
#include <queue>
#include <type_traits>
#include <stdexcept>

#include <SFML/Graphics.hpp>

#include "client_game.h"
#include "graphics.h"
#include "com_client.h"
#include "protocol.h"
#include "serialization.h"
#include "utility.h"
#include "command_gui.h"
#include "target_gui.h"
#include "upgrades.h"
#include "research.h"
#include "desktop.h"
#include "development_gui.h"
#include "solar_gui.h"

using namespace std;
using namespace st3;
using namespace client;

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
  targui = 0;
  selector_queue = 1;
  chosen_quit = false;
  comgui_active = false;

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

function<int(sf::Event)> game::generate_event_handler(function<int(sf::Event)> task){
  return [&task, this] (sf::Event e) {
    bool had_qw = !!interface::desktop -> query_window;
    interface::desktop -> HandleEvent(e);

    // handle escape on query window
    if (had_qw) {
      if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Escape){
	interface::desktop -> clear_qw();
      }
      return 0;
    }

    if (e.type == sf::Event::Closed) {
      // handle close event
      window.close();
      return query_aborted;
    }else{
      // handle custom task
      return task(e);
    }
  };
}

command_selector::ptr game::get_command_selector(idtype i){
  if (!command_selectors.count(i)) throw runtime_error("client::game::get_command_selectors: invalid id: " + to_string(i));
  return command_selectors[i];
}

entity_selector::ptr game::get_entity(combid i){
  if (!entity.count(i)) throw runtime_error("client::game::get_entity: invalid id: " + i);
  return entity[i];
}

template<typename T>
typename specific_selector<T>::ptr game::get_specific(combid i){
  auto e = get_entity(i);

  if (e -> isa(T::class_id)){
    return utility::guaranteed_cast<specific_selector<T>, entity_selector>(e);
  }else{
    throw runtime_error("get_specific: " + T::class_id + ": wrong class id for: " + i);
  }
}

template<typename T>
set<typename specific_selector<T>::ptr> game::get_all(){
  set<typename specific_selector<T>::ptr> s;
  typename specific_selector<T>::ptr test;
  
  for (auto x : entity){
    if (x.second -> isa(T::class_id)) s.insert(get_specific<T>(x.first));
  }
  
  return s;
}

void game::clear_guis(){
  if (targui) delete targui;
  targui = 0;
}

/* Main game entry point called after getting id from server */
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
  interface::desktop = new interface::main_interface(window.getSize(), this);

  init_data();
  
  // game loop
  while (true){
    clear_guis();
    if (!pre_step()) break;
    if (!choice_step()) break;
    if (!simulation_step()) break;
  }

  // message to user on involontary quit
  if (!chosen_quit){
    string server_says;
    socket -> data >> server_says;
    popup_message("GAME OVER", server_says);
  }
  
  delete interface::desktop;
  interface::desktop = 0;
}

bool game::wait_for_it(sf::Packet &p){
  int done = 0;
  
  thread t(query, socket, ref(p), ref(done));
  window_loop(done, default_event_handler, default_body);
  t.join();

  if (done & (query_game_complete | query_aborted)){
    cout << "wait_for_it: finished/aborted" << endl;
    return false;
  }

  return true;
}

bool game::init_data(){
  sf::Packet pq;
  data_frame data;

  message = "loading players...";
  pq << protocol::load_init;

  if (!(wait_for_it(pq) && deserialize(data, socket -> data, self_id))) {
    throw runtime_error("pre_step: failed to load/deserialize init_data");
  }

  players = data.players;
  settings = data.settings;
  col = sf::Color(players[self_id].color);
  
  for (auto i : data.entity){
    if (i.second -> isa(solar::class_id) && i.second -> owned){
      view_game.setCenter(i.second -> get_position());
      view_game.setSize(point(25 * settings.solar_maxrad, 25 * settings.solar_maxrad));
      break;
    }
  }

  return true;
}

/**First step in game round.
   Responsible for retrieving data from server and running reload_data.   
*/

bool game::pre_step(){
  sf::Packet pq;
  data_frame data;

  message = "loading game data...";
  pq << protocol::game_round;

  cout << "client " << self_id << ": pre step: start" << endl;
  if (!(wait_for_it(pq) && deserialize(data, socket -> data, self_id))) {
    cout << "pre_step: failed to load/deserialize game_data" << endl;
    return false;
  }
  cout << "client " << self_id << ": pre step: loaded" << endl;

  reload_data(data);

  for (auto x : entity) {
    if (x.second -> isa(solar::class_id)) {
      solar_selector::ptr s = get_specific<solar>(x.first);
      s -> flag = !s -> available_facilities(players[s -> owner].research_level).empty();
    }
  }

  return true;
}

/** Second step in game round.

    Responsible for running the choice gui, building a choice object
    and sending it to the server.
*/

bool game::choice_step(){

  // reset interface response parameters and clear solar choices for
  // solars which are no longer available.
  interface::desktop -> done = false;
  interface::desktop -> accept = false;

  choice::choice c;
  for (auto x : interface::desktop -> response.solar_choices){
    if (entity.count(x.first) && get_entity(x.first) -> owned) {
      x.second.development = "";
      c.solar_choices[x.first] = x.second;
    }
  }
  interface::desktop -> response = c;

  cout << "choice_step: start" << endl;

  // check if we can select a technology
  if (!players[self_id].research_level.available().empty()) {
    interface::development_gui::f_req_t f_req = [this] (string v) -> list<string> {
      return players[self_id].research_level.list_tech_requirements(v);
    };

    interface::development_gui::f_complete_t on_complete = [this] (bool accepted, string result) {
      if (accepted) {
	interface::desktop -> response.research = result;
	players[self_id].research_level.researched.insert(interface::desktop -> response.research);
      }
      
      interface::desktop -> clear_qw();
    };
    
    interface::desktop -> reset_qw(interface::development_gui::Create(research::data::table(), f_req, on_complete, false));
  }

  message = "make your choice";

  // event handler that passes the event to choice_event()
  auto event_handler = generate_event_handler([this](sf::Event e) -> int {
      // check the target gui
      window.setView(view_game);
      if (targui && targui -> handle_event(e)){
	if (targui -> done){
	  if (targui -> selected_option.key != "cancel"){
	    combid k = targui -> selected_option.key;
	    bool postselect = false;
	    
	    if (k == "add_waypoint"){
	      k = add_waypoint(targui -> position);
	      postselect = true;
	    }

	    command2entity(k, targui -> selected_option.option, targui -> selected_entities);

	    if (postselect) {
	      deselect_all();
	      get_entity(k) -> selected = true;
	    }
	  }
	  delete targui;
	  targui = 0;
	}
	return 0;
      }

      // check the command gui
      window.setView(view_window);

      // process choice event
      return choice_event(e);
    });

  // gui loop body that handles return status from interface::desktop
  auto body = [this] () -> int {
    if (interface::desktop -> done) {
      return interface::desktop -> accept ? query_accepted : query_game_complete;
    }
    return default_body();
  };

  int done = 0;
  window_loop(done, event_handler, body);

  sf::Packet pq;

  // client chose to leave the game
  if (done & (query_game_complete | query_aborted)){
    cout << "choice_step: finishded" << endl;
    pq.clear();
    pq << protocol::leave;
    while (!socket -> send_packet(pq)) sf::sleep(sf::milliseconds(100));
    return false;
  }

  // clear guis while sending choice
  clear_guis();
  deselect_all();

  message = "sending choice to server...";

  // add commands to choice
  c = build_choice(interface::desktop -> response);
  pq << protocol::choice << c;

  cout << "client " << self_id << ": choice step: start" << endl;
  if (!wait_for_it(pq)){
    cout << "choice send: server says game finished!" << endl;
    return false;
  }

  cout << "client " << self_id << ": choice step: loaded" << endl;
  return true;
}

/** Third game step.

    Responsible for visualizing game frames.
*/

bool game::simulation_step(){
  int done = 0;
  bool playing = true;
  int idx = -1;
  int loaded = 0;
  vector<data_frame> g(settings.frames_per_round);

  cout << "simluation: starting data loader" << endl;

  // start loading frames in background
  thread t(load_frames, socket, ref(g), ref(loaded), ref(done));

  // event handler which handles play/pause
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

  // gui loop body that draws progress indicator and keeps track of
  // the active frame
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
      float w = interface::main_interface::desktop_dims.x - 20;
      auto rect = graphics::build_rect(sf::FloatRect(10, interface::main_interface::top_height + 10, r * w, 20));
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

    if (playing){
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

/** Add a waypoint
    
    Creates a waypoint_selector at a given point.

    @param p the point
    @return the waypoint id
*/
combid game::add_waypoint(point p){
  waypoint buf(self_id);
  buf.position = p;
  waypoint_selector::ptr w = waypoint_selector::create(buf, col, true);
  entity[w -> id] = w;

  cout << "added waypoint " << w -> id << endl;

  return w -> id;
}

/** Fills a choice object with data.

    Looks through the entity_selector data and adds commands and
    waypoints to the choice.

    @return the modified choice
*/
choice::choice game::build_choice(choice::choice c){
  cout << "client " << self_id << ": build choice:" << endl;
  for (auto x : entity){
    for (auto y : x.second -> commands){
      if (command_selectors.count(y)){
	c.commands[x.first].push_back(*get_command_selector(y));
	cout << "Adding command for entity " << x.first << endl;
      }else{
	throw runtime_error("Attempting to build invalid command: " + y);
      }
    }

    if (x.second -> isa(waypoint::class_id)) {
      c.waypoints[x.first] = *get_specific<waypoint>(x.first);
      cout << "build choice: " << x.first << endl;
    }

    if (x.second -> isa(fleet::class_id)) {
      c.fleets[x.first] = *get_specific<fleet>(x.first);
      cout << "build_choice: " << x.first << endl;
    }
  }
  
  return c;
}

/** List all commands incident to an entity by key.
 */
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

void game::remove_entity(combid i){
  auto e = get_entity(i);
  auto buf = e -> commands;
  for (auto c : buf) remove_command(c);
  delete get_entity(i);
  entity.erase(i);
}

/** Load new game data from a data_frame.

    Adds and removes entity selectors given by the frame. Also adds
    new command selectors representing fleet commands.
*/
void game::reload_data(data_frame &g){
  // make selectors 'not seen' and 'not owned' and clear commands and
  // waypoints
  clear_selectors();  
  players = g.players;
  settings = g.settings;

  // update entities: fleets, solars and waypoints
  for (auto x : g.entity){
    entity_selector::ptr p = x.second;
    combid key = x.first;
    if (entity.count(key)) remove_entity(key);
    entity[key] = p;
    p -> seen = true;
    if (p -> owner == self_id && p -> is_active()) add_fixed_stars (p -> position, p -> vision());
    cout << "reload_data: loaded seen entity: " << p -> id << endl;
  }

  // remove entities as server specifies
  for (auto x : g.remove_entities){
    if (entity.count(x)){
      // if ship, add explosion
      auto p = get_entity(x);
      if (p -> is_active() && p -> isa(ship::class_id)) {
	explosions.push_back(explosion(p -> position, players[p -> owner].color));
      }
      
      cout << " -> remove entity " << x << endl;
      remove_entity(x);
    }
  }

  // remove unseen entities that are in sight range
  list<combid> rbuf;
  for (auto y : entity){
    entity_selector::ptr s = y.second;
    if (!s -> seen){
      for (auto x : entity){
	if (x.second -> owned && utility::l2norm(s -> position - x.second -> position) < x.second -> vision()){
	  rbuf.push_back(s -> id);
	  cout << "reload_data: spotted unseen ship: " << s -> id << endl;
	  break;
	}
      }
    }
  }
  for (auto id : rbuf) remove_entity(id);

  // fix fleet positions
  for (auto f : get_all<fleet>()) {
    point p(0,0);
    for (auto sid : f -> get_ships()) p += get_entity(sid) -> position;
    f -> position = utility::scale_point(p, 1 / (float)f -> get_ships().size());
  }

  // update commands for owned fleets
  for (auto f : get_all<fleet>()) {
    if (entity.count(f -> com.target) && f -> owned){
      // include commands even if f is idle e.g. to waypoint
      command c = f -> com;
      point to = get_entity(f -> com.target) -> get_position();
      // assure we don't assign ships which have been killed
      c.ships = c.ships & f -> ships;
      add_command(c, f -> position, to, false);
    } else {
      f -> com.target = identifier::target_idle;
      f -> com.action = fleet_action::idle;
    }
  }

  // update commands for waypoints
  for (auto w : get_all<waypoint>()){
    for (auto c : w -> pending_commands){
      if (entity.count(c.target)){
	add_command(c, w -> get_position(), get_entity(c.target) -> get_position(), false);
      }
    }
    w -> pending_commands.clear();
  }

  // update research level ref for solars
  for (auto s : get_all<solar>()) s -> research_level = &players[s -> owner].research_level;
}

// ****************************************
// COMMAND MANIPULATION
// ****************************************

/** Add a command selector.
 */
void game::add_command(command c, point from, point to, bool fill_ships){
  // check if command already exists
  for (auto x : command_selectors){
    if (c == (command)*x.second) return;
  }

  entity_selector::ptr s = get_entity(c.source);
  entity_selector::ptr t = get_entity(c.target);

  // check waypoint ancestors
  if (waypoint_ancestor_of(c.target, c.source)){
    cout << "add_command: circular waypoint graphs forbidden!" << endl;
    return;
  }

  command_selector::ptr cs = command_selector::create(c, from, to);
  
  // add command to command selectors 
  command_selectors[cs -> id] = cs;

  // add ships to command
  if (fill_ships) {
    set<combid> ready_ships = get_ready_ships(c.source);
    cs -> ships = ready_ships;
  }

  // add command selector key to list of the source entity's children
  s -> commands.insert(cs -> id);
}

bool game::waypoint_ancestor_of(combid ancestor, combid child){
  // only waypoints can have this relationship
  if (identifier::get_type(ancestor).compare(waypoint::class_id) || identifier::get_type(child).compare(waypoint::class_id)){
    return false;
  }

  // if the child is the ancestor, it has been found!
  if (ancestor == child) return true;

  // if the ancestor is ancestor to a parent, it is ancestor to the child
  for (auto x : incident_commands(child)){
    if (waypoint_ancestor_of(ancestor, get_command_selector(x) -> source)) return true;
  }

  return false;
}

/** Remove a command.

    Also recursively remove empty waypoints and their child commands.
*/
void game::remove_command(idtype key){
  if (!command_selectors.count(key)) return;
  if (comgui_active) interface::desktop -> clear_qw();
  comgui_active = false;
  
  command_selector::ptr cs = get_command_selector(key);
  entity_selector::ptr s = get_entity(cs -> source);
  entity_selector::ptr t = get_entity(cs -> target);

  // remove this command's selector
  delete cs;
  command_selectors.erase(key);

  // remove this command from it's source's list
  s -> commands.erase(key);

  // if last command of waypoint, remove it
  if (t -> isa(waypoint::class_id)){
    if (incident_commands(t -> id).empty()){
      remove_entity(t -> id);
    }
  }
}

// ****************************************
// SELECTOR MANIPULATION
// ****************************************

/** Mark all entity selectors as not seen/owned and clear command
    selectors and waypoints. */
void game::clear_selectors(){
  for (auto x : entity) {
    x.second -> seen = false;
    x.second -> owned = false;
  }
  
  comid = 0;
  for (auto c : command_selectors) delete c.second;
  command_selectors.clear();

  for (auto w : get_all<waypoint>()) remove_entity(w -> id);
}

/** Mark all entity and command selectors as not selected. */
void game::deselect_all(){
  for (auto x : entity) x.second -> selected = false;
  for (auto x : command_selectors) x.second -> selected = false;
}

// ****************************************
// EVENT HANDLING
// ****************************************

/** Mark all area selectable entity selectors in selection rectangle as selected. */
void game::area_select(){
  sf::FloatRect rect = fixrect(srect);

  if (!add2selection()) deselect_all();  
  for (auto x : entity){
    x.second -> selected = x.second -> owned && x.second -> is_area_selectable() && x.second -> inside_rect(rect);
  }
}

/** Create commands targeting an entity. 
    
    @param key target entity id
    @param act command action
    @param selected_entities selected entities
*/
void game::command2entity(combid key, string act, list<combid> e_selected){
  if (!entity.count(key)) throw runtime_error("command2entity: invalid key: " + key);

  command c;
  point from, to;
  c.target = key;
  c.action = act;
  to = get_entity(key) -> get_position();

  for (auto x : e_selected){
    if (entity.count(x) && x != key){
      entity_selector::ptr s = get_entity(x);
      if (s -> is_commandable()){
	c.source = x;
	from = s -> get_position();
	add_command(c, from, to, true);
      }
    }
  }
}

/** List all entities at a point. */
list<combid> game::entities_at(point p){
  float d;
  list<combid> keys;

  // find entities at p
  for (auto x : entity){
    if (x.second -> is_active() && x.second -> contains_point(p, d)) keys.push_back(x.first);
  }

  return keys;
}

/** Get queued owned entity at a point. */ 
combid game::entity_at(point p, int &q){
  list<combid> buf = entities_at(p);
  list<combid> keys;
  
  // limit to owned selectable entities
  for (auto id : buf) {
    auto e = get_entity(id);
    if (e -> owner == self_id && e -> is_selectable()) keys.push_back(id);
  }

  if (keys.empty()) return identifier::source_none;

  keys.sort([this] (combid a, combid b) -> bool {
      return get_entity(a) -> queue_level < get_entity(b) -> queue_level;
  });

  combid best = keys.front();
  q = get_entity(best) -> queue_level;
  return best;
}

/** Get queued command at a point. */
idtype game::command_at(point p, int &q){
  int qmin = selector_queue;
  float d;
  idtype key = -1;

  // find next queued command near p
  for (auto x : command_selectors){
    if (x.second -> contains_point(p, d) && x.second -> queue_level < qmin){
      qmin = x.second -> queue_level;
      key = x.first;
    }
  }

  q = qmin;
  return key;
}

/** Select the next queued entity or command selector at a point. */
bool game::select_at(point p){
  int qent, qcom;
  combid key = entity_at(p, qent);
  idtype cid = command_at(p, qcom);

  if (qcom < qent) return select_command(cid);

  auto it = entity.find(key);
 
  if (!add2selection()) deselect_all();

  if (it != entity.end() && it -> second -> owned){
    it -> second -> selected = !(it -> second -> selected);
    it -> second -> queue_level = selector_queue++;
    return true;
  }

  return false;
}

/** Select a command. 
    
    Sets up the command gui.
*/
bool game::select_command(idtype key){ 
  if (!add2selection()) deselect_all();
  if (!command_selectors.count(key)) return false;
  auto c = get_command_selector(key);

  c -> selected = !(c -> selected);
  c -> queue_level = selector_queue++;

  if (!c -> selected) return false;

  interface::desktop -> reset_qw(interface::command_gui::Create(c, this));
  comgui_active = true;
  return true;
}

/** At least one selected entity? */
bool game::exists_selected(){
  for (auto x : entity) if (x.second -> selected) return true;
  return false;
}

/** Get ids of non-allocated ships for entity selector */
set<combid> game::get_ready_ships(combid id){
  if (!entity.count(id)) throw runtime_error("get ready ships: entity selector " + id + " not found!");

  entity_selector::ptr e = get_entity(id);
  set<combid> s = e -> get_ships();
  for (auto c : e -> commands) s -= get_command_selector(c) -> ships;

  return s;
}

/** Get ids of selected solars */
template<typename T>
list<combid> game::selected_specific(){
  list<combid> res;
  for (auto s : get_all<T>()){
    if (s -> selected){
      res.push_back(s -> id);
    }
  }
  return res;
}

/** Get ids of selected entities. */
list<combid> game::selected_entities(){
  list<combid> res;
  for (auto &x : entity){
    if (x.second -> selected) res.push_back(x.first);
  }
  return res;
}

list<idtype> game::selected_commands(){
  list<idtype> res;
  for (auto i : command_selectors) {
    if (i.second -> selected) res.push_back(i.first);
  }
  return res;
}

/** Start the solar gui. */
void game::run_solar_gui(combid key){
  interface::desktop -> reset_qw(interface::solar_gui::Create(get_specific<solar>(key)));
}

void game::setup_targui(point p){
  // set up targui
  auto keys_targeted = entities_at(p);
  auto keys_selected = selected_entities();
  auto ships_selected = selected_specific<ship>();

  // remove ships from selection
  for (auto sid : ships_selected) keys_selected.remove(sid);
  if (keys_selected.empty()) return;
  
  list<target_gui::option_t> options;
  set<string> possible_actions;

  // add possible actions from available ship interactions
  for (auto k : keys_selected){
    entity_selector::ptr e = get_entity(k);
    for (auto i : e -> get_ships()){
      ship::ptr s = get_specific<ship>(i);
      for (auto u : s -> upgrades) possible_actions += upgrade::table().at(u).inter;
    }
  }

  // check if actions are allowed per target
  auto itab = interaction::table();
  for (auto a : possible_actions){
    auto condition = itab[a].condition.owned_by(self_id);
    for (auto k : keys_targeted){
      if (condition.valid_on(get_entity(k))){
	options.push_back(target_gui::option_t(k, a));
      }
    }
  }

  // check waypoint targets
  for (auto k : keys_targeted){
    if (identifier::get_type(k) == waypoint::class_id){
      options.push_back(target_gui::option_t(k, fleet_action::go_to));
    }
  }

  if (options.empty()){
    // autoselect "add waypoint"
    combid k = add_waypoint(p);
    command2entity(k, fleet_action::go_to, keys_selected);
    deselect_all();
    get_entity(k) -> selected = true;
  }else{
    // default options
    options.push_back(target_gui::option_add_waypoint);
    options.push_back(target_gui::option_cancel);

    targui = new target_gui(p, options, keys_selected, &window);
  }
} 

/** Event handler for the main choice interface.

    Called by the event handler in choice_step.

    @return true if client is finished
*/
int game::choice_event(sf::Event e){
  point p;
  list<combid> ss;

  sf::Vector2i mpos;
  sf::FloatRect minirect;
  point delta;
  point target;

  // event reaction functions
  auto init_area_select = [this] (sf::Event e) {
    point p = window.mapPixelToCoords(sf::Vector2i(e.mouseButton.x, e.mouseButton.y));
    area_select_active = true;
    srect = sf::FloatRect(p.x, p.y, 0, 0);
  };

  auto update_hover_info = [this] (point p) {
    auto keys = entities_at(p);
    string text = "Empty space";
      
    if (keys.size() == 1){
      combid k = *keys.begin();
      if (!entity.count(k)) throw runtime_error("update hover info: entity not in table: " + k);
      text = get_entity(k) -> hover_info();
    }else if (keys.size() > 1){
      text = "Multiple entities here!";
    }

    interface::desktop -> hover_label -> SetText(text);
  };

  // delete all selected fleets and commands
  auto handle_delete = [this] () {
    for (auto id : selected_commands()) remove_command(id);
    for (auto id : selected_specific<fleet>()) {
      fleet_selector::ptr f = get_specific<fleet>(id);
      for (auto sid : f -> get_ships()) get_specific<ship>(sid) -> fleet_id = identifier::source_none;
      remove_entity(id);
    }
  };

  // make a fleet from selected ships
  auto make_fleet = [this](){
    auto buf = selected_specific<ship>();
    deselect_all();
    
    if (!buf.empty()) {
      fleet fb(self_id);
      fleet_selector::ptr f = fleet_selector::create(fb, sf::Color(players[self_id].color), true);
      f -> ships = set<combid>(buf.begin(), buf.end());
      float vis = 0;
      point pos(0,0);
      for (auto sid : buf) {
	ship_selector::ptr s = get_specific<ship>(sid);
	s -> fleet_id = f -> id;
	vis = fmax(vis, s -> vision());
	pos += s -> position;
      }

      f -> radius = settings.fleet_default_radius;
      f -> stats.vision_buf = vis;
      f -> position = utility::scale_point(pos, 1 / (float)buf.size());
      f -> selected = true;
      
      entity[f -> id] = f;
    }
  };


  // event switch
  window.setView(view_game);
  switch (e.type){
  case sf::Event::MouseButtonPressed:
    if (e.mouseButton.button == sf::Mouse::Left) init_area_select(e);
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
	  view_game.setCenter(target);
	}else{
	  select_at(p);
	}
      }
    } else if (e.mouseButton.button == sf::Mouse::Right && exists_selected()){
      setup_targui(p);
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
    }else{
      update_hover_info(p);
    }
    break;
  case sf::Event::MouseWheelMoved:
    break;
  case sf::Event::KeyPressed:
    switch (e.key.code){
    case sf::Keyboard::Space:
      if (targui){
	clear_guis();
      }else{
	return query_accepted;
      }
      break;
    case sf::Keyboard::Return:
      ss = selected_specific<solar>();
      if (ss.size() == 1) run_solar_gui(ss.front());
      break;
    case sf::Keyboard::Delete:
      handle_delete();
      break;
    case sf::Keyboard::F:
      make_fleet();
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

/** Update virtual camera based on key controls. */
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

/** Draw a box with a message and wait for ok. */
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

  auto event_handler = generate_event_handler([this] (sf::Event e) -> int {
      if (e.type == sf::Event::KeyPressed){
	if (e.key.code == sf::Keyboard::Return){
	  return query_accepted;
	}
      }
      return 0;
    });

  window_loop(done, event_handler, default_body);

  interface::desktop -> Remove(w);
}

/** Draw a box with a a query and options ok or cancel, wait for response. */
bool game::popup_query(string v){
  hm_t<string, string> options;
  options["ok"] = "Ok";
  options["cancel"] = "Cancel";
  return popup_options(v, options) == "ok";
}

string game::popup_options(string header_text, hm_t<string, string> options) {
  int done = false;
  string response;

  auto w = sfg::Window::Create();
  auto layout = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 10);
  auto blayout = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 10);
  auto header = sfg::Label::Create(header_text);

  sfg::Button::Ptr button;

  for (auto v : options) {
    button = sfg::Button::Create(v.second);
    button -> GetSignal(sfg::Widget::OnLeftClick).Connect([v, &done, &response] () {
	response = v.first;
	done = true;
      });
    blayout -> Pack(button);
  }

  layout -> Pack(header);
  layout -> Pack(blayout);
  w -> Add(layout);
  interface::desktop -> Add(w);

  w -> SetPosition(sf::Vector2f(window.getSize().x / 2 - w -> GetRequisition().x / 2, window.getSize().y / 2 - w -> GetRequisition().y / 2));

  auto event_handler = generate_event_handler([this, &response] (sf::Event e) -> int {
      if (e.type == sf::Event::KeyPressed){
	if (e.key.code == sf::Keyboard::Escape){
	  response = "";
	  return query_aborted;
	}
      }
      return 0;
    });

  window_loop(done, event_handler, default_body);

  interface::desktop -> Remove(w);
  
  return response;
}

/** Core loop for gui. */
void game::window_loop(int &done, function<int(sf::Event)> event_handler, function<int(void)> body){
  sf::Clock clock;
  float frame_time = 1/(float)20;

  while (!done){
    auto delta = clock.restart().asSeconds();

    sf::Event event;
    while (window.pollEvent(event) && !done){
      done |= event_handler(event);
    }
    
    if (!window.isOpen()) done |= query_aborted;

    if (done) break;

    // update controls
    controls();

    // draw universe and game objects
    window.clear();
    draw_window();

    // main content callback
    done |= body();

    if (done) break;

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
}

/** Draw universe and game objects on the window */
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
  if (interface::desktop -> query_window == 0){
    // Interface stuff that is only drawn if there is no query window

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
    text.setFillColor(sf::Color(200,200,200));
    text.setString(message);
    text.setPosition(point(10, 0.2 * (interface::main_interface::desktop_dims.y)));
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

/** Get positional rectangle representing the minimap in the window. */
sf::FloatRect game::minimap_rect(){
  sf::FloatRect fr = view_minimap.getViewport();
  sf::FloatRect r;
  r.left = fr.left * view_window.getSize().x;
  r.top = fr.top * view_window.getSize().y;
  r.width = fr.width * view_window.getSize().x;
  r.height = fr.height * view_window.getSize().y;
  return r;
}

/** Draw command selectors and selection rectangle. */
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

/** Draw entities, explosions and stars. */
void game::draw_universe(){
  for (auto star : fixed_stars) star.draw(window);

  list<explosion> buf;
  for (auto e : explosions) {
    graphics::draw_explosion(window, e);
    if (e.time_passed() < 4) buf.push_back(e);
  }
  explosions = buf;
  
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
