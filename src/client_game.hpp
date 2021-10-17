#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "choice.hpp"
#include "client_data_frame.hpp"
#include "fixed_star.hpp"
#include "game_base_data.hpp"
#include "rsg/src/RskTypes.hpp"
#include "selector.hpp"
#include "types.hpp"

namespace st3 {

const float frame_time = 0.05;

struct cl_socket_t;

/*! Client game interface

      The game struct runs the client game interface. It stores the
      window, socket, game settings and selectors representing game
      objects, as well as sub guis. 
    */
class game : public game_base_data {
 private:
  enum gui_layers {
    LAYER_BASE,
    LAYER_CONTEXT,
    LAYER_PANEL,
    LAYER_POPUP,
    LAYER_NUM
  };

  enum game_phase {
    SIMULATION,
    CHOICE,
    INITIALIZATION
  };

  // Server communication
  std::shared_ptr<cl_socket_t> socket; /*!< socket for server communication */

  // Window views
  sf::View view_game;    /*!< sfml view for game objects */
  sf::View view_minimap; /*!< sfml view for the minimap */
  sf::View view_window;  /*!< sfml view fitting window */

  // GUI static components
  std::vector<RSG::PanelPtr> component_layers;

  // RSG interface variables
  bool is_loading;
  bool area_select_active; /*!< whether area selection is active */
  sf::FloatRect srect;     /*!< area selection rectangle */
  int selector_queue;      /*!< index for back end of selector queue */
  std::list<std::string> event_log;
  std::string hover_info_title;
  std::list<std::string> hover_info_items;
  RSG::ProgressBarPtr sim_progress;
  RSG::ButtonPtr sim_generated_label;

  // Interface logic variables
  bool state_run;
  game_phase phase;
  bool choice_complete;
  choice user_choice;
  std::list<RSG::Voidfun> ui_tasks;
  std::mutex ui_task_mutex;
  std::mutex game_data_mutex;
  bool did_drag;
  bool drag_waypoint_active;
  bool drag_map_active;
  std::set<idtype> selection_index;

  // Game variables
  sf::Color col;
  sint self_id;
  std::vector<point> enemy_clusters;

  // Simulation data
  int sim_frames_loaded;
  sint frames_generated;
  int sim_idx;
  int sim_sub_idx;
  bool sim_playing;
  std::vector<client_data_frame> sim_frames;

  // Command selectors
  idtype comid;                                          /*!< id counter for commands */
  hm_t<idtype, command_selector::ptr> command_selectors; /*!< graphical representations for commands */

  // stars
  std::vector<fixed_star> fixed_stars;
  std::list<fixed_star> hidden_stars;
  static constexpr float grid_size = 20;
  std::set<std::pair<int, int>> known_universe;
  point sight_ul;
  point sight_wh;

  // animations
  std::list<animation> animations;

  // Background tasks
  hm_t<int, std::shared_ptr<std::future<void>>> background_tasks;
  std::mutex background_task_mutex;

 public:
  static std::weak_ptr<game> gmain;
  RSG::WindowPtr window; /*!< sfml window for drawing the interface */

  game(std::shared_ptr<cl_socket_t> s, RSG::WindowPtr w);
  void terminate_with_message(std::string message);
  void run();

 private:
  // OBJECT ACCESS
  void deregister_entity(idtype i);
  research::data get_research() const;
  command_selector::ptr get_command_selector(idtype i) const;
  std::vector<entity_selector::ptr> all_selectors() const;
  std::list<idtype> solars_for_panel() const;

  // USER INTERFACE RELATED STUFF
  void window_loop();
  void queue_ui_task(RSG::Voidfun f);
  void process_ui_tasks();
  void do_clear_ui_layers(bool preserve_base = true);
  bool any_gui_content(int level = LAYER_CONTEXT) const;

  // METHODS THAT ENQUEUE A UI MODIFICATION
  void set_loading(bool s);
  void push_game_log(std::list<std::string> log);
  void set_hover_info(std::string title, std::list<std::string> info);
  void swap_layer(int layer, RSG::ComponentPtr component);
  void set_main_panel(RSG::PanelPtr p);
  void clear_ui_layers(bool preserve_base = true);
  void build_base_panel();
  void popup_query(std::string title, std::string v, hm_t<std::string, RSG::Voidfun> opts);
  void popup_message(std::string title, std::string text);

  // METHODS THAT PRODUCE A UI COMPONENT
  RSG::PanelPtr research_gui();
  RSG::PanelPtr development_gui();
  RSG::PanelPtr military_gui();
  RSG::PanelPtr event_log_widget();
  RSG::PanelPtr hover_info_widget();
  RSG::PanelPtr simulation_gui();

  // SERVER COMMUNICATION AND BACKGROUND TASKS
  void target_selected(std::string action, idtype target, point pos, std::list<idtype> e_sel);
  void prepare_data_frame(client_data_frame &g);
  void load_frames();

  /*! Update entities to correspond to the current sim frame */
  void update_sim_frame();

  /*! Increment simulation to next index */
  void next_sim_frame();

  /*! Add a task to be run in background thread */
  void queue_background_task(RSG::Voidfun f);

  void check_background_tasks();

  /*! Queue background task: send a packet to query and callback with response */
  void wait_for_it(packet_ptr p, std::function<void(sf::Packet &)> callback, RSG::Voidfun on_fail = 0);

  /*! Tell server we are leaving game */
  void tell_server_quit(RSG::Voidfun callback, RSG::Voidfun on_fail);

  // STEP SETUP FUNCTIONS

  /*! Start background task: load init data from server */
  void init_data();

  void pre_step();
  void choice_step();
  void send_choice();
  void simulation_step();

  // UNSORTED METHODS BELOW THIS POINT

  void reset_drags();

  void update_sight_range(point p, float r);

  /*! make a choice from the user interface
	@return the choice
      */
  choice build_choice();

  /*! update gui with new game data
	@param g the game data
      */
  void reload_data(client_data_frame &g, bool use_animations = true);

  // event handling
  /*! update the choice generating gui with an sfml event
	@param e the event
	@return client::query_status denoting whether to proceed to next step
      */
  bool choice_event(sf::Event e);
  void control_event(sf::Event e);

  void do_zoom(float factor, point p);

  std::function<int(sf::Event)> generate_event_handler(std::function<int(sf::Event)> task);
  std::function<int()> generate_loop_body(std::function<int()> task);

  /*! start a solar gui for a solar
	@param key the id of the solar selector representing the solar
      */
  void run_solar_gui(idtype key);

  /*! select selectors in the selection rectangle */
  void area_select();

  /*! get the keys of all entity selectors at a point

	@param p the point
	@return set of keys of entities
      */
  std::list<idtype> entities_at(point p);

  /*! get the key of the entity selector at a point

	If there are entities at the point, this function identifies
	the entity with the lowest queue level, increments the queue
	level and returns the key.

	@param p the point
	@param[out] q set to queue level of found entity
	@return the key, or "" if no entity was found
      */
  idtype entity_at(point p, int &q);

  /*! get the id of the command_selector at a point
	@param p the point
	@param[out] q set to queue level of found command
	@return the id
      */
  idtype command_at(point p, int &q);

  void set_selected(idtype id, bool value);

  /*! select the selector at a point if there is one
	@param p the point
	@return whether an entity was selected
      */
  bool select_at(point p);

  /*! setup the command gui
	
	select the command selector and set up the command gui.

	@param key id of the command selector
	@return whether a command selector was selected
      */
  bool select_command(idtype key);

  /*! translate the game view according to user input */
  void controls();

  /*! check whether a waypoint is an ancestor of another waypoint

	This is used to prevent the user from generating circular
	graphs.

	@param ancestor key of the potential ancestor
	@param child key to check the ancestor of
	@return whether waypoint ancestor is an ancestor of waypoint child
      */
  bool waypoint_ancestor_of(idtype ancestor, idtype child);

  /* **************************************** */
  /* COMMAND HANDLING */
  /* **************************************** */

  // Refresh available downstream ships for an entity
  void refresh_ships(idtype id);

  /*! add a command selector representing a command
	@param c the command
	@param from the coordinate point of the source
	@param to the coordinate point of the target
	@param fill_ships whether to automatically allocate the ready ships from the source
      */
  void add_command(command c, point from, point to, bool fill_ships = false, bool default_policy = true);

  /*! remove a command selector
	@param key id of the command selector to remove
      */
  void remove_command(idtype key);

  /*! add commands from selected entities to a target entity
	@param key entity to target
	@param action command action
	@param list of selected entities
      */
  void command2entity(idtype key, std::string action, std::list<idtype> sel);

  /*! add a waypoint selector at a given point
	@param p the point
	@return key of the added waypoint selector
      */
  idtype add_waypoint(point p);

  /* **************************************** */
  /* SELECTION HANDLING */
  /* **************************************** */

  /*! list all selectors as not seen and delete all command selectors */
  void clear_selectors();

  /*! mark all entity and command selectors as not selected */
  void deselect_all();

  /*! get the ids of all command selectors targeting an entity selector 
	@param key key of the entity selector
	@return list of ids of commands targeting the entity selector
      */
  std::list<idtype> incident_commands(idtype key);

  /*! count the number of selected entity selectors
	@return the number of selected entity selectors
      */
  bool exists_selected();

  /*! get the ready ships of an entity selector
	
	Get the ids of ships associated to an entity selector which
	are not allocated to a command selector.

	@param key key of the entity selector
	@return set of ship ids
      */
  hm_t<std::string, std::set<idtype>> get_ready_ships(idtype key);

  /*! get a list of keys of all selected entity selectors
	@return list of keys of selected entity selectors
      */
  std::list<idtype> selected_entities();

  std::list<idtype> selected_commands();

  /* **************************************** */
  /* GRAPHICS */
  /* **************************************** */

  sf::FloatRect minimap_rect();

  void setup_targui(point p);

  /*! draw the gui
	
	Draws the universe, interface components, message and minimap 
      */
  void draw_window();

  /*! draw entity selectors and ships */
  void draw_minimap();
  void draw_universe();

  /*! draw command selectors, area selection rect and command gui */
  void draw_interface_components();

  void add_fixed_stars(point position, float vision);

  template <typename T>
  typename specific_selector<T>::ptr get_specific(idtype i) {
    return get_entity<specific_selector<T>>(i);
  };

  template <typename T>
  std::vector<typename specific_selector<T>::ptr> get_all(idtype pid = game_object::any_owner) const {
    return filtered_entities<specific_selector<T>>(pid);
  };

  entity_selector::ptr get_selector(idtype i) const {
    return get_entity<entity_selector>(i);
  }

  /** Get ids of selected solars */
  template <typename T>
  std::list<idtype> selected_specific() const {
    std::list<idtype> res;
    for (auto s : get_all<T>()) {
      if (s->selected) {
        res.push_back(s->id);
      }
    }
    return res;
  };
};
};  // namespace st3
