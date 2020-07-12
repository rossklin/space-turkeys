#include "client_game.hpp"

#include <SFML/Graphics.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>

#include "animation.hpp"
#include "choice_gui.hpp"
#include "com_client.hpp"
#include "command_gui.hpp"
#include "graphics.hpp"
#include "protocol.hpp"
#include "research.hpp"
#include "rsg/src/button.hpp"
#include "rsg/src/panel.hpp"
#include "rsg/src/utility.hpp"
#include "selector.hpp"
#include "serialization.hpp"
#include "socket_t.hpp"
#include "solar_gui.hpp"
#include "style.hpp"
#include "target_gui.hpp"
#include "upgrades.hpp"
#include "utility.hpp"

using namespace std;
using namespace st3;
using namespace RSG;

// local utility functions
sf::FloatRect fixrect(sf::FloatRect r);
bool add2selection();
bool ctrlsel();

// ****************************************
// GAME STEPS
// ****************************************

game::game(std::shared_ptr<cl_socket_t> s) {
  sim_sub_frames = 4;
  selector_queue = 1;
  sight_ul = point(0, 0);
  sight_wh = point(0, 0);
  socket = s;
  area_select_active = false;
  self_id = socket->id;
  fleet_idc = 0;
  wp_idc = 0;
  component_layers.resize(LAYER_NUM);
}

vector<entity_selector::ptr> game::all_selectors() const {
  return all_entities<entity_selector>();
};

command_selector::ptr game::get_command_selector(idtype i) {
  if (!command_selectors.count(i)) throw classified_error("client::game::get_command_selectors: invalid id: " + to_string(i));
  return command_selectors[i];
}

void game::clear_ui_layers(bool preserve_base) {
  queue_ui_task(bind(&do_clear_ui_layers, this, preserve_base));
}

void game::do_clear_ui_layers(bool preserve_base) {
  auto make_container = styled_generator<Panel>(
      {
          {"width", to_string(window.getSize().x) + "px"},
          {"height", to_string(window.getSize().y) + "px"},
          {"align-horizontal", "center"},
          {"align-vertical", "center"},
      });

  for (int i = 0; i < LAYER_NUM; i++) {
    component_layers[i] = make_container();
  }

  if (!preserve_base) base_layer = make_container();
}

/* Main game entry point called after getting id from server */
void game::run() {
  bool proceed = true;
  bool first = true;

  init_data();

  // construct interface
  view_window = window.getDefaultView();
  view_minimap.setViewport(sf::FloatRect(0.01, 0.71, 0.28, 0.28));
  window.setView(view_window);

  // Setup all layers, do not preserve base layer
  do_clear_ui_layers(false);

  // todo
  window_loop();
}

// Start background task: query server with packet, then callback with result
void game::wait_for_it(sf::Packet &p, function<void(sf::Packet &)> callback, RSG::Voidfun on_fail) {
  queue_background_task([this, &p, callback, on_fail]() {
    if (client::query(socket, p)) {
      callback(socket->data);
    } else if (on_fail) {
      on_fail();
    } else {
      terminate_with_message("Error sending query to server!");
    }
  });
}

// Load initial data from the server
void game::init_data() {
  set_loading(true);

  auto callback = [this](sf::Packet p) {
    game_base_data data;

    try {
      client::deserialize(data, p, self_id);
    } catch (exception e) {
      terminate_with_message("Failed to load init data");
      return;
    }

    players = data.players;
    settings = data.settings;
    col = sf::Color(players[self_id].color);

    sight_ul = point(-settings.clset.galaxy_radius, -settings.clset.galaxy_radius);
    sight_wh = point(2 * settings.clset.galaxy_radius, 2 * settings.clset.galaxy_radius);
    update_sight_range(point(0, 0), 1);

    // load player starting positions
    for (auto s : data.filtered_entities<solar_selector>()) {
      s->research_level = &players[s->owner].research_level;
      add_entity(s);
      cout << "init_data: added: " << s->id << endl;

      if (s->owned) {
        view_game.setCenter(s->get_position());
        view_game.setSize(point(25 * settings.solar_meanrad, 25 * settings.solar_meanrad));
        s->seen = true;
        cout << "init_data: selected starting position: " << s->id << endl;
      }
    }

    pre_step();
  };

  auto on_fail = [this]() { terminate_with_message("Load init data: can't reach server!"); };

  sf::Packet pq;
  pq << protocol::load_init;

  wait_for_it(pq, callback, on_fail);
}

/**First step in game round.
   Responsible for retrieving data from server and running reload_data.   
*/

void game::pre_step() {
  clear_ui_layers();
  set_loading(true);
  sf::Packet pq;
  phase = "pre";
  pq << protocol::game_round;

  auto callback = [this](sf::Packet p) -> bool {
    game_base_data data;
    try {
      client::deserialize(data, p, self_id);
    } catch (exception e) {
      terminate_with_message("Pre-step: failed to load: " + string(e.what()));
      return;
    }
    cout << "client " << self_id << ": pre step: loaded" << endl;

    reload_data(data, false);

    set_loading(false);
    choice_step();
  };

  auto on_fail = [this]() { terminate_with_message("Load pre step data: can't reach server!"); };

  wait_for_it(pq, callback, on_fail);
}

void game::target_selected(string action, combid target, point pos, list<string> e_sel) {
  bool postselect = false;

  if (target == identifier::source_none) {
    target = add_waypoint(pos);
    postselect = true;
  }

  command2entity(target, action, e_sel);

  if (postselect) {
    deselect_all();
    get_selector(target)->selected = true;
  }

  clear_ui_layers();
}

void game::choice_step() {
  phase = "choice";
  build_base_panel();

  // keep solar and research choices
  choice c;
  research::data &r = players[self_id].research_level;
  c.research = r.researching;

  // check if we can select a technology
  if (c.research.empty() || r.access(c.research).level > 0) {
    // somehow we have already researched this, e.g. found in seach mission
    c.research.clear();
  }

  cout << "choice_step: start" << endl;

  if (c.research.empty() && !r.available().empty()) {
    swap_layer(LAYER_PANEL, research_gui());
  }

  choice_complete = false;

  // start standby com thread
  queue_background_task([this]() {
    sf::Packet p;
    p << protocol::standby;

    while (!choice_complete) {
      if (client::query(socket, p)) {
        sf::sleep(sf::milliseconds(500));
      } else {
        terminate_with_message("Choice standby thread: network error");
        return;
      }
    }

    send_choice();
  });
}

void game::send_choice() {
  // clear guis while sending choice
  clear_ui_layers(false);
  deselect_all();

  choice c = build_choice();
  sf::Packet pq;
  pq << protocol::choice << c;

  wait_for_it(pq, [this](sf::Packet &data) {
    simulation_step();
  });
}

/** Third game step.

    Responsible for visualizing game frames.
*/

void game::simulation_step() {
  phase = "simulation";
  cout << "simluation: starting data loader" << endl;

  // start loading frames in background
  sim_frames_loaded = 0;
  sim_sub_idx = 0;
  sim_idx = 0;
  sim_playing = false;
  queue_background_task(bind(&game::load_frames, this));

  clear_ui_layers(false);
  swap_layer(LAYER_PANEL, simulation_gui());
}

void game::next_sim_frame() {
  sim_sub_idx++;
  if (sim_sub_idx >= sim_sub_frames) {
    int buffer_size = min<int>(settings.clset.frames_per_round - sim_idx - 1, 4);
    if (sim_idx < sim_frames_loaded - buffer_size) {
      sim_sub_idx = 0;
      sim_idx++;
      reload_data(sim_frames[sim_idx]);
    } else {
      sim_playing = false;
      return;
    }
  }

  for (auto &a : animations) a.frame++;
}

// Update entities to correspond to the current sim frame
void game::update_sim_frame() {
  // static int sub_idx = 1;
  // float sub_ratio = 1 / (float)sub_frames;
  int total_frames = sim_sub_frames * settings.clset.frames_per_round;

  // if (idx == settings.clset.frames_per_round - 1) {
  //   cout << "simulation: all loaded" << endl;
  //   return socket_t::tc_complete;
  // }

  // // draw load progress
  // window.setView(view_window);

  // auto colored_rect = [](sf::Color c, float r) -> sf::RectangleShape {
  //   float bounds = interface::main_interface::desktop_dims.x;
  //   float w = bounds - 10;
  //   auto rect = graphics::build_rect(sf::FloatRect(5, 5, r * w, 10));
  //   c.a = 150;
  //   rect.setFillColor(c);
  //   rect.setOutlineColor(sf::Color::White);
  //   rect.setOutlineThickness(-1);
  //   return rect;
  // };

  // window.draw(colored_rect(sf::Color::Red, 1));
  // window.draw(colored_rect(sf::Color::Blue, loaded / (float)settings.clset.frames_per_round));
  // window.draw(colored_rect(sf::Color::Green, idx / (float)settings.clset.frames_per_round));

  // message = "evolution: " + to_string((100 * idx) / settings.clset.frames_per_round) + " %" + (playing ? "" : "(paused)");

  // if (playing) {

  float t = sim_sub_idx / (float)sim_sub_frames;
  float bw = 1;

  // update entity positions
  for (auto sh : get_all<ship>()) {
    if (sh->seen) {
      vector<point> pbuf(4);
      vector<float> abuf(4);

      auto load_buffers = [&, this](int offset) -> bool {
        for (int i = -1; i < 3; i++) {
          int idx_access = sim_idx + offset + i;
          if (idx_access < 0 || idx_access >= sim_frames_loaded) return false;
          if (!g[idx_access].entity_exists(sh->id)) return false;

          entity_selector::ptr ep = g[idx_access].get_entity<entity_selector>(sh->id);
          if (!ep->is_active()) return false;

          pbuf[1 + i] = ep->base_position;
          abuf[1 + i] = ep->base_angle;
        }
        return true;
      };

      int offset = 0;
      bool test = false;
      for (auto i : utility::zig_seq(2)) {
        if (test = load_buffers(offset = i)) break;
      }

      if (test) {
        sh->position = utility::cubic_interpolation(pbuf, t - offset);
        sh->angle = utility::cubic_interpolation(abuf, t - offset);
      } else {
        sh->position = sh->base_position + t * sh->velocity;
      }
    }
  }

  for (auto fl : get_all<fleet>()) {
    point p(0, 0);
    for (auto sid : fl->ships) p += get_specific<ship>(sid)->position;
    fl->position = utility::scale_point(p, 1 / (float)fl->ships.size());
  }

  for (auto cs : command_selectors) {
    cs.second->from = get_selector(cs.second->source)->position;
    cs.second->to = get_selector(cs.second->target)->position;
  }

  for (auto &a : animations) {
    auto update_tracker = [this](animation_tracker_info &t) {
      if (entity_exists(t.eid)) {
        t.p = get_selector(t.eid)->position;
      } else {
        // Todo
        // t.p = sub_ratio * t.v;
      }
    };

    update_tracker(a.t1);
    update_tracker(a.t2);
  }
}

// ****************************************
// DATA HANDLING
// ****************************************

/** Add a waypoint
    
    Creates a waypoint_selector at a given point.

    @param p the point
    @return the waypoint id
*/
combid game::add_waypoint(point p) {
  waypoint buf(self_id, wp_idc++);
  buf.position = p;
  waypoint_selector::ptr w = waypoint_selector::create(buf, col, true);
  add_entity(w);

  cout << "added waypoint " << w->id << endl;

  return w->id;
}

/** Fills a choice object with data.

    Looks through the entity_selector data and adds commands and
    waypoints to the choice.

    @return the modified choice
*/
choice game::build_choice() {
  choice c = user_choice;
  cout << "client " << self_id << ": build choice:" << endl;
  for (auto x : all_selectors()) {
    for (auto y : x->commands) {
      if (command_selectors.count(y)) {
        c.commands[x->id].push_back(*get_command_selector(y));
        cout << "Adding command for entity " << x->id << endl;
      } else {
        throw classified_error("Attempting to build invalid command: " + y);
      }
    }

    if (x->isa(waypoint::class_id)) {
      c.waypoints[x->id] = get_specific<waypoint>(x->id);
      cout << "build choice: " << x->id << endl;
    } else if (x->isa(fleet::class_id)) {
      c.fleets[x->id] = get_specific<fleet>(x->id);
      cout << "build_choice: " << x->id << endl;
    } else if (x->isa(solar::class_id) && x->owned) {
      c.solar_choices[x->id] = get_specific<solar>(x->id)->choice_data;
      cout << "build_choice: " << x->id << endl;
    }
  }

  return c;
}

/** List all commands incident to an entity by key.
 */
list<idtype> game::incident_commands(combid key) {
  list<idtype> res;

  for (auto x : command_selectors) {
    if (x.second->target == key) {
      res.push_back(x.first);
    }
  }

  return res;
}

void game::update_sight_range(point position, float r) {
  point sight_br = sight_ul + sight_wh;
  sight_ul.x = fmin(sight_ul.x, position.x - r);
  sight_ul.y = fmin(sight_ul.y, position.y - r);
  sight_br.x = fmax(sight_br.x, position.x + r);
  sight_br.y = fmax(sight_br.y, position.y + r);
  sight_wh = sight_br - sight_ul;

  view_minimap.setCenter(sight_ul + utility::scale_point(sight_wh, 0.5));
  view_minimap.setSize(sight_wh);
}

// written by Johan Mattsson
void game::add_fixed_stars(point position, float vision) {
  float r = vision + grid_size;
  update_sight_range(position, vision);

  for (float p = -r; p < r; p += grid_size) {
    float ymax = sqrt(r * r - p * p);
    for (float j = -ymax; j < ymax; j += grid_size) {
      int grid_x = round((position.x + p) / grid_size);
      int grid_y = round((position.y + j) / grid_size);
      pair<int, int> grid_index(grid_x, grid_y);

      for (int i = rand() % 3; i >= 0; i--) {
        if (known_universe.count(grid_index) == 0) {
          point star_position;
          star_position.x = grid_index.first * grid_size + utility::random_uniform() * grid_size;
          star_position.y = grid_index.second * grid_size + utility::random_uniform() * grid_size;

          fixed_star star(star_position);

          if (utility::l2norm(star_position - position) < vision) {
            fixed_stars.push_back(star);
          } else {
            hidden_stars.push_back(star);
          }

          known_universe.insert(grid_index);
        }
      }
    }
  }

  auto i = hidden_stars.begin();
  while (i != hidden_stars.end()) {
    fixed_star star = *i;
    i++;
    if (utility::l2norm(star.position - position) < vision) {
      fixed_stars.push_back(star);
      hidden_stars.remove(star);
    }
  }
}

void game::deregister_entity(combid i) {
  auto e = get_selector(i);
  auto buf = e->commands;
  for (auto c : buf) remove_command(c);
  remove_entity(i);
}

/** Load new game data from a data_frame.

    Adds and removes entity selectors given by the frame. Also adds
    new command selectors representing fleet commands.
*/
void game::reload_data(game_base_data &g, bool use_animations) {
  // make selectors 'not seen' and 'not owned' and clear commands and
  // waypoints
  clear_selectors();
  players = g.players;
  settings = g.settings;
  terrain = g.terrain;

  // update entities: fleets, solars and waypoints
  for (auto a : g.all_entities<entity_selector>()) {
    entity_selector::ptr p = a->ss_clone();
    combid key = p->id;
    if (entity_exists(key)) deregister_entity(key);
    add_entity(p);
    p->seen = p->is_active();
    if (p->owner == self_id && p->is_active()) add_fixed_stars(p->position, p->vision());
    cout << "reload_data for " << self_id << ": loaded seen entity: " << p->id << " owned by " << p->owner << endl;
  }

  // remove entities as server specifies
  for (auto x : g.remove_entities) {
    if (entity_exists(x)) {
      cout << " -> remove entity " << x << endl;
      deregister_entity(x);
    }
  }

  // remove unseen ships that are in sight range
  list<combid> rbuf;
  for (auto s : get_all<ship>()) {
    if (s->is_active() && !s->seen) {
      for (auto x : all_selectors()) {
        if (x->owned && utility::l2norm(s->position - x->position) < x->vision()) {
          rbuf.push_back(s->id);
          cout << "reload_data: spotted unseen ship: " << s->id << endl;
          break;
        }
      }
    }
  }
  for (auto id : rbuf) deregister_entity(id);

  // fix fleet positions
  for (auto f : get_all<fleet>()) {
    point p(0, 0);
    for (auto sid : f->get_ships()) p += get_selector(sid)->position;
    f->position = utility::scale_point(p, 1 / (float)f->get_ships().size());
  }

  // update commands for owned fleets
  for (auto f : get_all<fleet>()) {
    fleet_idc = max(fleet_idc, identifier::get_multid_index(f->id) + 1);

    if (entity_exists(f->com.target) && f->owned) {
      // include commands even if f is idle e.g. to waypoint
      command c = f->com;
      point to = get_selector(f->com.target)->get_position();
      // assure we don't assign ships which have been killed
      c.ships = c.ships & f->ships;
      add_command(c, f->position, to, false, false);
    } else {
      f->com.target = identifier::target_idle;
      f->com.action = fleet_action::idle;
    }
  }

  // update commands for waypoints
  for (auto w : get_all<waypoint>()) {
    wp_idc = max(wp_idc, identifier::get_multid_index(w->id) + 1);

    for (auto c : w->pending_commands) {
      if (entity_exists(c.target)) {
        add_command(c, w->get_position(), get_selector(c.target)->get_position(), false, false);
      }
    }
    w->pending_commands.clear();
  }

  // update research level ref for solars
  for (auto s : get_all<solar>()) {
    if (s->owned) {
      s->research_level = &players[s->owner].research_level;
    } else {
      s->research_level = NULL;
    }
  }

  // add animations
  if (use_animations) {
    for (auto &a : players[self_id].animations) animations.push_back(a);
  } else {
    animations.clear();
  }

  // update enemy cluster buffer
  enemy_clusters.clear();
  for (auto s : get_all<ship>()) {
    if (s->seen && !s->owned) enemy_clusters.push_back(s->position);
  }
  if (enemy_clusters.size()) enemy_clusters = utility::cluster_points(enemy_clusters, 20, 100);
}

// ****************************************
// COMMAND MANIPULATION
// ****************************************

/** Add a command selector.
 */
void game::add_command(command c, point from, point to, bool fill_ships, bool default_policy) {
  // check if command already exists
  for (auto x : command_selectors) {
    if (c == (command)*x.second) return;
  }

  entity_selector::ptr s = get_selector(c.source);
  entity_selector::ptr t = get_selector(c.target);

  // check waypoint ancestors
  if (waypoint_ancestor_of(c.target, c.source)) {
    cout << "add_command: circular waypoint graphs forbidden!" << endl;
    return;
  }

  // check that there is at least one ship available
  hm_t<string, set<combid>> ready_ships = get_ready_ships(c.source);
  if (fill_ships && ready_ships.empty()) {
    cout << "add_command: attempted to create empty command!" << endl;
    return;
  }

  // check that we respect fleet count limit
  if (filtered_entities<fleet>(self_id).size() >= get_max_fleets(self_id)) {
    cout << "add_command: reached max nr of fleets!" << endl;
    return;
  }

  // set default fleet policy
  if (default_policy) c.policy = fleet::default_policy(c.action);

  command_selector::ptr cs = command_selector::create(c, from, to);

  // add command to command selectors
  command_selectors[cs->id] = cs;

  // add ships to command
  if (fill_ships) {
    set<string> fleet_actions = {fleet_action::go_to, interaction::land, interaction::space_combat, interaction::bombard};

    // check if special action
    if (!fleet_actions.count(c.action)) {
      combid sel = identifier::source_none;
      for (auto ship_set : ready_ships) {
        for (auto sid : ship_set.second) {
          if (get_specific<ship>(sid)->compile_interactions().count(c.action)) {
            sel = sid;
            break;
          }
        }
        if (sel != identifier::source_none) break;
      }

      if (sel != identifier::source_none) {
        ready_ships.clear();
        ready_ships[get_specific<ship>(sel)->ship_class].insert(sel);
      }
    }

    // only add ships of one class
    cs->ship_class = utility::hm_keys(ready_ships).front();
    set<combid> sel_ships = ready_ships[cs->ship_class];
    cs->ships.clear();
    for (auto sid : sel_ships) {
      cs->ships.insert(sid);
      if (cs->ships.size() >= get_max_ships_per_fleet(self_id)) break;
    }
  }

  // add command selector key to list of the source entity's children
  s->commands.insert(cs->id);
}

bool game::waypoint_ancestor_of(combid ancestor, combid child) {
  // only waypoints can have this relationship
  if (identifier::get_type(ancestor).compare(waypoint::class_id) || identifier::get_type(child).compare(waypoint::class_id)) {
    return false;
  }

  // if the child is the ancestor, it has been found!
  if (ancestor == child) return true;

  // if the ancestor is ancestor to a parent, it is ancestor to the child
  for (auto x : incident_commands(child)) {
    if (waypoint_ancestor_of(ancestor, get_command_selector(x)->source)) return true;
  }

  return false;
}

/** Remove a command.

    Also recursively remove empty waypoints and their child commands.
*/
void game::remove_command(idtype key) {
  if (!command_selectors.count(key)) return;
  clear_ui_layers();

  command_selector::ptr cs = get_command_selector(key);
  entity_selector::ptr s = get_selector(cs->source);
  entity_selector::ptr t = get_selector(cs->target);

  // remove this command's selector
  command_selectors.erase(key);

  // remove this command from it's source's list
  s->commands.erase(key);

  // if last command of waypoint, remove it
  if (t->isa(waypoint::class_id)) {
    if (incident_commands(t->id).empty()) {
      deregister_entity(t->id);
    }
  }
}

// ****************************************
// SELECTOR MANIPULATION
// ****************************************

/** Mark all entity selectors as not seen/owned and clear command
    selectors and waypoints. */
void game::clear_selectors() {
  for (auto x : all_selectors()) {
    x->seen = false;
    x->owned = false;
  }

  comid = 0;
  command_selectors.clear();

  for (auto w : get_all<waypoint>()) deregister_entity(w->id);
}

/** Mark all entity and command selectors as not selected. */
void game::deselect_all() {
  for (auto x : all_selectors()) x->selected = false;
  for (auto x : command_selectors) x.second->selected = false;
}

// ****************************************
// EVENT HANDLING
// ****************************************

/** Mark all area selectable entity selectors in selection rectangle as selected. */
void game::area_select() {
  sf::FloatRect rect = fixrect(srect);

  if (!add2selection()) deselect_all();
  for (auto x : all_selectors()) {
    x->selected = x->owned && x->is_area_selectable() && x->inside_rect(rect);
  }
}

/** Create commands targeting an entity. 
    
    @param key target entity id
    @param act command action
    @param selected_entities selected entities
*/
void game::command2entity(combid key, string act, list<combid> e_selected) {
  if (!entity_exists(key)) throw classified_error("command2entity: invalid key: " + key);

  command c;
  point from, to;
  c.target = key;
  c.action = act;
  to = get_selector(key)->get_position();

  for (auto x : e_selected) {
    if (entity_exists(x) && x != key) {
      entity_selector::ptr s = get_selector(x);
      if (s->is_commandable()) {
        c.source = x;
        from = s->get_position();
        add_command(c, from, to, true);
      }
    }
  }
}

/** List all entities at a point. */
list<combid> game::entities_at(point p) {
  float d;
  list<combid> keys;

  // find entities at p
  for (auto x : all_selectors()) {
    if (x->is_active() && x->contains_point(p, d)) keys.push_back(x->id);
  }

  return keys;
}

/** Get queued owned entity at a point. */
combid game::entity_at(point p, int &q) {
  list<combid> buf = entities_at(p);
  list<combid> keys;

  // limit to owned selectable entities
  for (auto id : buf) {
    auto e = get_selector(id);
    if (e->owned && e->is_selectable()) keys.push_back(id);
  }

  if (keys.empty()) return identifier::source_none;

  keys.sort([this](combid a, combid b) -> bool {
    return get_selector(a)->queue_level < get_selector(b)->queue_level;
  });

  combid best = keys.front();
  q = get_selector(best)->queue_level;
  return best;
}

/** Get queued command at a point. */
idtype game::command_at(point p, int &q) {
  int qmin = selector_queue;
  float d;
  idtype key = -1;

  // find next queued command near p
  for (auto x : command_selectors) {
    if (x.second->contains_point(p, d) && x.second->queue_level < qmin) {
      qmin = x.second->queue_level;
      key = x.first;
    }
  }

  q = qmin;
  return key;
}

/** Select the next queued entity or command selector at a point. */
bool game::select_at(point p) {
  int qent = selector_queue;
  int qcom = selector_queue;
  combid key = entity_at(p, qent);
  idtype cid = command_at(p, qcom);

  if (qcom < qent && phase == "choice") return select_command(cid);

  if (!add2selection()) deselect_all();

  if (entity_exists(key)) {
    entity_selector::ptr e = get_selector(key);
    if (e->owned) {
      e->selected = !(e->selected);
      e->queue_level = selector_queue++;
      return true;
    }
  }

  return false;
}

/** Select a command. 
    
    Sets up the command gui.
*/
bool game::select_command(idtype key) {
  if (!add2selection()) deselect_all();
  if (!command_selectors.count(key)) return false;
  auto c = get_command_selector(key);

  c->selected = !(c->selected);
  c->queue_level = selector_queue++;

  if (!c->selected) return false;

  set<combid> ready_ships = get_ready_ships(c->source)[c->ship_class];
  set<combid> all_ships = ready_ships + c->ships;

  bool combat = false;
  if (all_ships.size()) {
    combat = get_specific<ship>(*all_ships.begin())->compile_interactions().count(interaction::space_combat);
  }

  swap_layer(
      LAYER_PANEL,
      command_gui(
          c->ship_class,
          c->action,
          c->policy,
          all_ships.size(),
          combat,
          [this, c](int policy, int num) {
            c->policy = policy;
            c->ships.clear();
            vector<combid> ready_ships = utility::range_init<vector<combid>>(get_ready_ships(c->source)[c->ship_class]);
            if (num > ready_ships.size()) num = ready_ships.size();
            c->ships = set<combid>(ready_ships.begin(), ready_ships.begin() + num);
            c->selected = false;
            clear_ui_layers();
          },
          [this, c]() {
            c->selected = false;
            clear_ui_layers();
          }));

  return true;
}

/** At least one selected entity? */
bool game::exists_selected() {
  for (auto x : all_selectors()) {
    if (x->selected) return true;
  }
  return false;
}

/** Get ids of non-allocated ships for entity selector */
hm_t<string, set<combid>> game::get_ready_ships(combid id) {
  if (!entity_exists(id)) throw classified_error("get ready ships: entity selector " + id + " not found!");

  entity_selector::ptr e = get_selector(id);
  hm_t<string, set<string>> res;

  for (auto sid : e->get_ships()) res[get_specific<ship>(sid)->class_id].insert(sid);

  for (auto c : e->commands) {
    auto com = get_command_selector(c);
    if (res.count(com->ship_class)) {
      res[com->ship_class] -= com->ships;
    }
  }

  return res;
}

/** Get ids of selected entities. */
list<combid> game::selected_entities() {
  list<combid> res;
  for (auto e : all_selectors()) {
    if (e->selected) res.push_back(e->id);
  }
  return res;
}

list<idtype> game::selected_commands() {
  list<idtype> res;
  for (auto i : command_selectors) {
    if (i.second->selected) res.push_back(i.first);
  }
  return res;
}

// /** Start the solar gui. */
// void game::run_solar_gui(combid key){
//   interface::desktop -> reset_qw(interface::solar_gui::Create(get_specific<solar>(key)));
// }

bool game::in_terrain(point p) {
  // check if there is terrain here
  for (auto &x : terrain) {
    int j = x.second.triangle(p, 0);
    if (j > -1) {
      return true;
    }
  }

  return false;
}

void game::setup_targui(point p) {
  if (in_terrain(p)) return;

  // set up targui
  auto keys_targeted = entities_at(p);
  auto keys_selected = selected_entities();
  auto ships_selected = selected_specific<ship>();

  // remove ships from selection
  for (auto sid : ships_selected) keys_selected.remove(sid);
  if (keys_selected.empty()) return;

  set<string> possible_actions;

  // add possible actions from available ship interactions
  bool exists_ships = false;
  for (auto k : keys_selected) {
    auto rships = get_ready_ships(k);
    exists_ships |= rships.size() > 0;
    for (auto ship_set : rships) {
      for (auto i : ship_set.second) {
        possible_actions += get_specific<ship>(i)->compile_interactions();
      }
    }
  }

  // never generate command if there are no ships
  if (!exists_ships) {
    popup_message("Invalid command!", "All ships already assigned.");
    return;
  }

  hm_t<string, set<combid>> options;

  // check if actions are allowed per target
  auto itab = interaction::table();
  for (auto a : possible_actions) {
    auto condition = itab[a].condition.owned_by(self_id);
    for (auto k : keys_targeted) {
      if (condition.valid_on(get_selector(k))) {
        options[a].insert(k);
      }
    }
    if (!condition.requires_target()) {
      options[a].insert(identifier::source_none);
    }
  }

  // check waypoint targets
  for (auto k : keys_targeted) {
    if (identifier::get_type(k) == waypoint::class_id) {
      options[fleet_action::go_to].insert(k);
    }
  }

  if (options.empty()) {
    // autoselect "add waypoint"
    combid k = add_waypoint(p);
    command2entity(k, fleet_action::go_to, keys_selected);
    deselect_all();
    get_selector(k)->selected = true;
  } else {
    PanelPtr targui = target_gui(
        sf::Vector2f(window.mapCoordsToPixel(p, view_game)),
        options,
        [this, keys_selected, p](string action, string target) {
          target_selected(action, target, p, keys_selected);
        },
        bind(&game::clear_ui_layers, this, true));

    swap_layer(LAYER_CONTEXT, targui);
  }
}

void game::control_event(sf::Event e) {
  static point p_prev(0, 0);
  static bool drag_map_active = false;
  static bool drag_waypoint_active = false;
  static bool did_drag = false;
  static combid drag_id;

  point p;
  sf::Vector2i mpos;
  sf::FloatRect minirect;
  point delta;
  point target;

  drag_map_active &= sf::Keyboard::isKeyPressed(sf::Keyboard::LControl);

  auto update_hover_info = [this](point p) {
    auto keys = entities_at(p);
    list<string> text;

    if (keys.empty()) {
      keys = selected_entities();
      if (keys.empty()) {
        return;
      }
    }

    string title = "";
    if (keys.size() > 1) title = "Multiple entities";

    for (auto k : keys) {
      text.push_back(get_selector(k)->hover_info());
    }

    set_hover_info(title, text);
  };

  // event reaction functions
  auto init_area_select = [this](sf::Event e) {
    point p = window.mapPixelToCoords(sf::Vector2i(e.mouseButton.x, e.mouseButton.y));
    int qent;
    combid key = entity_at(p, qent);

    if (phase == "choice" && identifier::get_type(key) == waypoint::class_id) {
      drag_waypoint_active = true;
      drag_id = key;
    } else {
      area_select_active = true;
      srect = sf::FloatRect(p.x, p.y, 0, 0);
    }
  };

  window.setView(view_game);
  switch (e.type) {
    case sf::Event::MouseMoved:
      p = window.mapPixelToCoords(sf::Vector2i(e.mouseMove.x, e.mouseMove.y));

      if (area_select_active) {
        // update area selection
        srect.width = p.x - srect.left;
        srect.height = p.y - srect.top;
      } else if (phase == "choice" && drag_waypoint_active && !in_terrain(p)) {
        // update position
        auto wp = get_specific<waypoint>(drag_id);
        wp->position = p;

        // update target for incident commands
        list<idtype> inc = incident_commands(drag_id);
        for (auto cid : inc) get_command_selector(cid)->to = p;

        // update source for owned commands
        for (auto cid : wp->commands) get_command_selector(cid)->from = p;

        did_drag = true;
      } else if (drag_map_active) {
        view_game.move(p_prev - p);
      } else {
        update_hover_info(p);
      }
      break;
    case sf::Event::MouseButtonPressed:
      mpos = sf::Vector2i(e.mouseButton.x, e.mouseButton.y);
      p = window.mapPixelToCoords(mpos);
      if (e.mouseButton.button == sf::Mouse::Left) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
          p_prev = p;
          drag_map_active = true;
        } else {
          init_area_select(e);
        }
      }
      break;
    case sf::Event::MouseButtonReleased:
      mpos = sf::Vector2i(e.mouseButton.x, e.mouseButton.y);
      p = window.mapPixelToCoords(mpos);

      if (e.mouseButton.button == sf::Mouse::Left) {
        if (drag_waypoint_active && did_drag) {
          // do nothing
        } else if (abs(srect.width) > 5 || abs(srect.height) > 5) {
          area_select();
        } else {
          // check if on minimap
          minirect = minimap_rect();
          if (minirect.contains(mpos.x, mpos.y)) {
            delta = point(mpos.x - minirect.left, mpos.y - minirect.top);
            target = point(sight_ul.x + delta.x / minirect.width * sight_wh.x, sight_ul.y + delta.y / minirect.height * sight_wh.y);
            view_game.setCenter(target);
          } else {
            select_at(p);
          }
        }

        // clear selection rect
        drag_waypoint_active = false;
        did_drag = false;
        area_select_active = false;
        srect = sf::FloatRect(0, 0, 0, 0);
        drag_map_active = false;
      }

      break;
    case sf::Event::MouseWheelMoved:
      p = window.mapPixelToCoords(sf::Vector2i(e.mouseWheel.x, e.mouseWheel.y));
      do_zoom(pow(1.2, -e.mouseWheel.delta), p);
      break;
    case sf::Event::KeyPressed:
      switch (e.key.code) {
        case sf::Keyboard::O:
          do_zoom(1.2, view_game.getCenter());
          break;
        case sf::Keyboard::I:
          do_zoom(1 / 1.2, view_game.getCenter());
          break;
      }
  }
}

void game::do_zoom(float factor, point p) {
  point center = view_game.getCenter();
  point size = view_game.getSize();
  point ul = center - utility::scale_point(size, 0.5);
  point br = center + utility::scale_point(size, 0.5);
  point delta_ul = ul - p;
  point delta_br = br - p;
  point new_ul = p + utility::scale_point(delta_ul, factor);
  point new_br = p + utility::scale_point(delta_br, factor);
  point delta = new_br - new_ul;

  // check limits
  if (utility::l2norm(delta) < 50 || utility::l2norm(delta) > 10000) return;

  view_game.setCenter(utility::scale_point(new_ul + new_br, 0.5));
  view_game.setSize(new_br - new_ul);
}

// Choice step global handler, return true if event handled
bool game::choice_event(sf::Event e) {
  if (phase != "choice") return false;

  // // make a fleet from selected ships
  // auto make_fleet = [this]() {
  //   auto buf = selected_specific<ship>();
  //   deselect_all();

  //   if (!buf.empty()) {
  //     fleet fb(self_id, fleet_idc++);
  //     fleet_selector::ptr f = fleet_selector::create(fb, sf::Color(players[self_id].color), true);
  //     f->ships = set<combid>(buf.begin(), buf.end());
  //     float vis = 0;
  //     point pos(0, 0);
  //     for (auto sid : buf) {
  //       ship_selector::ptr s = get_specific<ship>(sid);
  //       s->fleet_id = f->id;
  //       vis = fmax(vis, s->vision());
  //       pos += s->position;
  //     }

  //     f->radius = settings.fleet_default_radius;
  //     f->stats.vision_buf = vis;
  //     f->position = utility::scale_point(pos, 1 / (float)buf.size());
  //     f->heading = f->position;
  //     f->selected = true;

  //     add_entity(f);
  //   }
  // };

  // auto solar_development = [this](string key) {
  //   bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl);
  //   bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift);
  //   auto ss = selected_specific<solar>();
  //   for (auto sid : ss) {
  //     solar_selector::ptr s = get_specific<solar>(sid);
  //     if (shift && ctrl) {
  //       // prepend to queue
  //       s->choice_data.building_queue.push_front(key);
  //     } else if (shift) {
  //       // append to queue
  //       s->choice_data.building_queue.push_back(key);
  //     } else if (ctrl) {
  //       // replace queue
  //       s->choice_data.building_queue = {key};
  //     }
  //   }
  // };

  // auto solar_production = [this](string key) {
  //   bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl);
  //   bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift);
  //   auto ss = selected_specific<solar>();
  //   for (auto sid : ss) {
  //     solar_selector::ptr s = get_specific<solar>(sid);
  //     if (shift && ctrl) {
  //       // prepend to queue
  //       s->choice_data.ship_queue.push_front(key);
  //     } else if (shift) {
  //       // append to queue
  //       s->choice_data.ship_queue.push_back(key);
  //     } else if (ctrl) {
  //       // replace queue
  //       s->choice_data.ship_queue = {key};
  //     }
  //   }
  // };

  // hm_t<int, string> ship_map;
  // hm_t<int, string> dev_map;

  // ship_map[sf::Keyboard::U] = "scout";
  // ship_map[sf::Keyboard::F] = "fighter";
  // ship_map[sf::Keyboard::B] = "bomber";
  // ship_map[sf::Keyboard::C] = "corsair";
  // ship_map[sf::Keyboard::A] = "cannon";
  // ship_map[sf::Keyboard::D] = "destroyer";
  // ship_map[sf::Keyboard::T] = "battleship";
  // ship_map[sf::Keyboard::V] = "voyager";
  // ship_map[sf::Keyboard::O] = "colonizer";
  // ship_map[sf::Keyboard::H] = "harvester";

  // dev_map[sf::Keyboard::A] = keywords::key_agriculture;
  // dev_map[sf::Keyboard::S] = keywords::key_shipyard;
  // dev_map[sf::Keyboard::D] = keywords::key_defense;
  // dev_map[sf::Keyboard::R] = keywords::key_research;

  point p;
  list<combid> ss;
  list<int> coms;

  sf::Vector2i mpos;
  sf::FloatRect minirect;
  point delta;
  point target;

  // Some global key commands are disabled when a UI panel is active
  bool has_gui = component_layers[LAYER_PANEL]->get_children().size();

  // delete all selected commands
  auto handle_delete = [this]() {
    for (auto id : selected_commands()) remove_command(id);
  };

  // event switch
  window.setView(view_game);
  switch (e.type) {
    // MOUSE BUTTON RELEASED
    case sf::Event::MouseButtonReleased:
      swap_layer(LAYER_CONTEXT, 0);
      mpos = sf::Vector2i(e.mouseButton.x, e.mouseButton.y);
      p = window.mapPixelToCoords(mpos);

      if (e.mouseButton.button == sf::Mouse::Right && exists_selected()) {
        setup_targui(p);
        return true;
      }

      break;

      // KEY PRESSED
    case sf::Event::KeyPressed:
      // // check build ship and development keys
      // if (activate_build && dev_map.count(e.key.code)) {
      //   solar_development(dev_map[e.key.code]);
      //   activate_build = false;
      //   break;
      // }

      // if (activate_ship && ship_map.count(e.key.code)) {
      //   solar_production(ship_map[e.key.code]);
      //   activate_ship = false;
      //   break;
      // }

      switch (e.key.code) {
        case sf::Keyboard::Space:
          if (!any_gui_content()) {
            choice_complete = true;
            return true;
          }
          break;
        case sf::Keyboard::Return:
          ss = selected_specific<solar>();
          if (ss.size() == 1) {
            auto sol = get_specific<solar>(ss.front());
            swap_layer(
                LAYER_PANEL,
                solar_gui(
                    sol,
                    get_research(),
                    bind(&game::clear_ui_layers, this, true),
                    [this, sol](list<string> qdev, list<string> qship) {
                      // Solar GUI callback
                      sol->choice_data.building_queue = qdev;
                      sol->choice_data.ship_queue = qship;
                      clear_ui_layers();
                    }));
            return true;
          }
          break;
        case sf::Keyboard::Delete:
          coms = selected_commands();
          if (coms.size()) {
            for (auto id : coms) remove_command(id);
            return true;
          }
          break;
        // case sf::Keyboard::B:
        //   if (selected_specific<solar>().size() > 0 && !has_gui) {
        //     activate_build = !activate_build;
        //     activate_ship = false;
        //   }
        //   break;
        // case sf::Keyboard::S:
        //   if (selected_specific<solar>().size() > 0 && !has_gui) {
        //     activate_ship = !activate_ship;
        //     activate_build = false;
        //   }
        //   break;
        case sf::Keyboard::Escape:
          if (any_gui_content()) {
            clear_ui_layers();
          } else {
            popup_query(
                "Really quit?",
                [this]() {
                  tell_server_quit([this]() { state_run = false; });
                },
                0);
          }
          return true;
      }
      break;
  };

  return false;
}

/** Update virtual camera based on key controls. */
void game::controls() {
  static point vel(0, 0);
  if (!window.hasFocus()) {
    vel = point(0, 0);
    return;
  }

  float s = view_game.getSize().x / sight_wh.x;
  sf::Vector2i mpos = sf::Mouse::getPosition(window);

  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) || mpos.x == 0) {
    vel.x -= 5 * s;
  } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) || mpos.x == window.getSize().x - 1) {
    vel.x += 5 * s;
  } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || mpos.y == 0) {
    vel.y -= 5 * s;
  } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) || mpos.y == window.getSize().y - 1) {
    vel.y += 5 * s;
  }

  vel = utility::scale_point(vel, 0.8);

  view_game.move(vel);
}

// ****************************************
// GRAPHICS
// ****************************************
using namespace graphics;

/** Draw a box with a message and wait for ok. */
void game::popup_message(string title, string message) {
  popup_query(title, message, {{"OK", [this]() { swap_layer(LAYER_POPUP, 0); }}});
}

/** Draw a box with a a query and options ok or cancel, wait for response. */
void game::popup_query(string title, string text, hm_t<string, Voidfun> opts) {
  PanelPtr bp = Panel::create();
  for (auto x : opts) bp->add_child(Button::create(x.first, x.second));

  swap_layer(
      LAYER_POPUP,
      Panel::create(
          {
              make_label(title),
              make_hbar(),
              make_label(text),
              make_hbar(),
              bp,
          },
          Panel::ORIENT_VERTICAL));
}

string game::popup_options(string header_text, hm_t<string, string> options) {
  int status = socket_t::tc_run;
  string response;

  auto w = sfg::Window::Create();
  auto layout = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 10);
  auto blayout = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 10);
  auto header = sfg::Label::Create(header_text);

  sfg::Button::Ptr button;

  for (auto v : options) {
    button = sfg::Button::Create(v.second);
    button->GetSignal(sfg::Widget::OnLeftClick).Connect([v, &status, &response]() {
      response = v.first;
      status = socket_t::tc_complete;
    });
    blayout->Pack(button);
  }

  layout->Pack(header);
  layout->Pack(blayout);
  w->Add(layout);
  interface::desktop->Add(w);

  w->SetPosition(sf::Vector2f(window.getSize().x / 2 - w->GetRequisition().x / 2, window.getSize().y / 2 - w->GetRequisition().y / 2));

  auto event_handler = generate_event_handler([this, &response](sf::Event e) -> int {
    if (e.type == sf::Event::KeyPressed) {
      if (e.key.code == sf::Keyboard::Escape) {
        response = "";
        return socket_t::tc_stop;
      }
    }
    return 0;
  });

  int result = 0;
  window_loop(event_handler, default_body, status, result);

  interface::desktop->Remove(w);

  return response;
}

/** Core loop for gui. */
void game::window_loop(function<int(sf::Event)> event_handler, function<int(void)> body, int &tc_in, int &tc_out) {
  chrono::time_point<chrono::system_clock> start;

  while ((tc_in | tc_out) == socket_t::tc_run) {
    start = chrono::system_clock::now();
    sf::Event event;
    while (window.pollEvent(event)) {
      tc_out |= event_handler(event);
    }

    if (!window.isOpen()) {
      tc_out |= socket_t::tc_stop;
      break;
    }

    window.clear();

    tc_out |= body();
    sfgui->Display(window);
    window.display();

    long int millis = 1000 * frame_time;
    this_thread::sleep_until(start + chrono::milliseconds(millis));
  }
}

/** Draw universe and game objects on the window */
void game::draw_window() {
  window.clear();

  // draw main interface
  window.setView(view_game);
  point ul = ul_corner(window);
  draw_universe();
  draw_interface_components();

  // draw targui
  window.setView(view_game);
  if (targui) {
    targui->draw();
  } else if (interface::desktop->query_window) {
    return;
  }

  // Interface stuff that is only drawn if there is no query window

  // draw minimap contents
  window.setView(view_minimap);
  draw_minimap();

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
  text.setCharacterSize(20);
  text.setFillColor(sf::Color(200, 200, 200));
  text.setString(message);
  text.setPosition(point(10, 20));
  window.draw(text);

  // draw fleet limits
  text.setString("Max fleets: " + to_string(get_max_fleets(self_id)));
  text.setPosition(point(10, 50));
  window.draw(text);
  text.setString("Max ships per fleet: " + to_string(get_max_ships_per_fleet(self_id)));
  text.setPosition(point(10, 80));
  window.draw(text);

  // draw minimap bounds
  sf::FloatRect fr = minimap_rect();
  r.setPosition(fr.left, fr.top);
  r.setSize(sf::Vector2f(fr.width, fr.height));
  r.setOutlineColor(sf::Color(255, 255, 255));
  r.setFillColor(sf::Color(0, 0, 25, 200));
  r.setOutlineThickness(1);
  window.draw(r);
}

/** Get positional rectangle representing the minimap in the window. */
sf::FloatRect game::minimap_rect() {
  sf::FloatRect fr = view_minimap.getViewport();
  sf::FloatRect r;
  r.left = fr.left * view_window.getSize().x;
  r.top = fr.top * view_window.getSize().y;
  r.width = fr.width * view_window.getSize().x;
  r.height = fr.height * view_window.getSize().y;
  return r;
}

/** Draw command selectors and selection rectangle. */
void game::draw_interface_components() {
  window.setView(view_game);

  // draw commands
  for (auto x : command_selectors) x.second->draw(window);

  if (area_select_active && srect.width && srect.height) {
    // draw selection rect
    sf::RectangleShape r = build_rect(srect);
    r.setFillColor(sf::Color(250, 250, 250, 50));
    r.setOutlineColor(sf::Color(80, 120, 240, 200));
    r.setOutlineThickness(1);
    window.draw(r);
  }
}

void game::draw_minimap() {
  for (auto e : all_selectors()) {
    if (!e->is_active()) continue;
    sf::FloatRect bounds(e->position, point(0.05 * window.getSize().x, 0.05 * window.getSize().y));
    sf::RectangleShape buf = graphics::build_rect(bounds, 0, sf::Color::Transparent, e->get_color());
    window.draw(buf);
  }
}

/** Draw entities, animations and stars. */
void game::draw_universe() {
  for (auto star : fixed_stars) star.draw(window);

  // draw terrain
  for (auto x : terrain) {
    terrain_object obj = x.second;
    int n = obj.border.size();
    sf::VertexArray polygon(sf::TriangleFan, n + 2);
    polygon[0].position = obj.center;
    polygon[0].color = sf::Color::Black;
    for (int i = 0; i < n; i++) {
      polygon[i + 1].position = obj.border[i];
      polygon[i + 1].color = sf::Color::Red;
    }
    polygon[n + 1] = polygon[1];
    window.draw(polygon);
  }

  list<animation> buf;
  for (auto e : animations) {
    graphics::draw_animation(window, e);
    if (e.time_passed() < animation::tmax) buf.push_back(e);
  }
  animations = buf;

  // draw fleets last
  for (auto e : all_selectors()) {
    if (!e->isa(fleet::class_id)) e->draw(window);
  }

  for (auto e : all_selectors()) {
    if (e->isa(fleet::class_id)) e->draw(window);
  }

  // flag clusters of enemy ships
  for (auto x : enemy_clusters) {
    graphics::draw_flag(window, x, sf::Color::Red, sf::Color::White, 0, "scout");
  }
}

// ****************************************
// UTILITY FUNCTIONS
// ****************************************

bool add2selection() {
  return sf::Keyboard::isKeyPressed(sf::Keyboard::LShift);
}

bool ctrlsel() {
  return sf::Keyboard::isKeyPressed(sf::Keyboard::LControl);
}

sf::FloatRect fixrect(sf::FloatRect r) {
  // fix reverse selection
  if (r.width < 0) {
    r.left += r.width;
    r.width *= -1;
  }

  if (r.height < 0) {
    r.top += r.height;
    r.height *= -1;
  }

  return r;
}
