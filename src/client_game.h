#ifndef _STK_CLIENTGAME
#define _STK_CLIENTGAME

#include <set>
#include <string>
#include <functional>
#include <SFML/Graphics.hpp>

#include "socket_t.h"
#include "fixed_star.h"
#include "graphics.h"
#include "choice.h"
#include "types.h"
#include "selector.h"
#include "com_client.h"
#include "data_frame.h"

namespace st3{
  class command_gui;
  class target_gui;
  
  namespace client{
    
    /*! Client game interface

      The game struct runs the client game interface. It stores the
      window, socket, game settings and selectors representing game
      objects, as well as sub guis. 
    */
    struct game : public data_frame{
      socket_t *socket; /*!< socket for server communication */
      window_t window; /*!< sfml window for drawing the interface */
      sf::View view_game; /*!< sfml view for game objects */
      sf::View view_minimap; /*!< sfml view for the minimap */
      sf::View view_window; /*!< sfml view fitting window */
      std::string message; /*!< game round progress message */
      bool area_select_active; /*!< whether area selection is active */
      sf::FloatRect srect; /*!< area selection rectangle */
      
      hm_t<idtype, command_selector::ptr> command_selectors; /*!< graphical representations for commands */
      int selector_queue; /*!< index for back end of selector queue */
      idtype comid; /*!< id counter for commands */
      sf::Color col;
      
      sfg::SFGUI *sfgui;
      command_gui *comgui; /*!< gui for assigning ships to commands */
      target_gui *targui; /*!< gui for selecting command action */
      bool chosen_quit;

      std::vector<fixed_star> fixed_stars;
      std::list<fixed_star> hidden_stars;
      static constexpr float grid_size = 20;
      std::set<std::pair<int, int> > known_universe;

      /*! default contsructor */
      game();

      entity_selector::ptr get_entity(combid i);

      template<typename T>
      typename specific_selector<T>::ptr get_specific(combid i);

      // round sections
      /*! run the game user interface */
      void run();

      /*! run the pre step: check with server and get game data
	@return whether to continue the game round
      */
      bool pre_step();

      /*! let user build a choice and send to server */
      bool choice_step();

      /*! load simulation frames from server and visualize */
      bool simulation_step();

      // data handling
      /*! make a command from a command selector
	@param key id of the command selector
	@return the command 
      */
      command build_command(idtype key);

      /*! make a choice from the user interface
	@return the choice
      */
      choice::choice build_choice(choice::choice c);

      /*! update gui with new game data
	@param g the game data
      */
      void reload_data(data_frame &g);

      // event handling
      /*! update the choice generating gui with an sfml event
	@param e the event
	@return client::query_status denoting whether to proceed to next step
      */
      int choice_event(sf::Event e);
      
      /*! start a solar gui for a solar
	@param key the id of the solar selector representing the solar
      */
      void run_solar_gui(combid key);

      /*! select selectors in the selection rectangle */
      void area_select();

      /*! get the keys of all entity selectors at a point

	@param p the point
	@return set of keys of entities
      */
      std::set<combid> entities_at(point p);

      /*! get the key of the entity selector at a point

	If there are entities at the point, this function identifies
	the entity with the lowest queue level, increments the queue
	level and returns the key.

	@param p the point
	@param[out] q set to queue level of found entity
	@return the key, or "" if no entity was found
      */
      combid entity_at(point p, int *q = 0);

      /*! get the id of the command_selector at a point
	@param p the point
	@param[out] q set to queue level of found command
	@return the id
      */
      idtype command_at(point p, int *q = 0);

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

      /*! remove the current command gui */
      void clear_guis();
      
      /*! recursively remove ships from a waypoint graph

	As the waypoints list any ships allocated to them from their
	ancestors in the waypoint graph, when removing ships from a
	waypoint they must also be removed recursively from this
	waypoint's children.

	@param wid entity selector key of the waypoint
	@param a set of ids of ships to remove
      */
      void recursive_waypoint_deallocate(combid wid, std::set<combid> a);

      /*! check whether a waypoint is an ancestor of another waypoint

	This is used to prevent the user from generating circular
	graphs.

	@param ancestor key of the potential ancestor
	@param child key to check the ancestor of
	@return whether waypoint ancestor is an ancestor of waypoint child
      */
      bool waypoint_ancestor_of(combid ancestor, combid child);

      /* **************************************** */
      /* COMMAND HANDLING */
      /* **************************************** */

      /*! check whether there already exists a command selector between entities

	@param c command to check source and target of
	@return whether there exists a command selector with same source/target as c
      */
      bool command_exists(command c);

      /*! get the id of a command selector matching a command

	Checks only source and target of the command.

	@param c the command
	@return the id
      */
      idtype command_id(command c);

      /*! add a command selector representing a command
	@param c the command
	@param from the coordinate point of the source
	@param to the coordinate point of the target
	@param fill_ships whether to automatically allocate the ready ships from the source
      */
      void add_command(command c, point from, point to, bool fill_ships = false);

      /*! remove a command selector
	@param key id of the command selector to remove
      */
      void remove_command(idtype key);

      /*! add commands from selected entities to a target entity
	@param key entity to target
	@param action command action
	@param list of selected entities
      */
      void command2entity(combid key, std::string action, std::list<std::string> sel);

      /*! add a waypoint selector at a given point
	@param p the point
	@return key of the added waypoint selector
      */
      combid add_waypoint(point p);

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
      std::list<idtype> incident_commands(combid key);

      /*! count the number of selected entity selectors
	@return the number of selected entity selectors
      */
      int count_selected();

      /*! get the ready ships of an entity selector
	
	Get the ids of ships associated to an entity selector which
	are not allocated to a command selector.

	@param key key of the entity selector
	@return set of ship ids
      */
      virtual std::set<combid> get_ready_ships(combid key);

      /*! get a list of keys of all selected solar selectors
	@return list of keys of selected solar selectors
      */
      std::list<combid> selected_solars();

      /*! get a list of keys of all selected entity selectors
	@return list of keys of selected entity selectors
      */
      std::list<combid> selected_entities();

      /* **************************************** */
      /* GRAPHICS */
      /* **************************************** */

      std::function<int(sf::Event)> default_event_handler;
      std::function<int()> default_body;

      sf::FloatRect minimap_rect();
      
      bool popup_query(std::string v);

      void popup_message(std::string title, std::string text);

      void window_loop(int &done, std::function<int(sf::Event)> event_handler, std::function<int(void)> body);

      /*! draw the gui
	
	Draws the universe, interface components, message and minimap 
      */
      void draw_window();

      /*! draw entity selectors and ships */
      void draw_universe();

      /*! draw command selectors, area selection rect and command gui */
      void draw_interface_components();
      
      void add_fixed_stars (point position, float vision);
    };
  };
};
#endif
