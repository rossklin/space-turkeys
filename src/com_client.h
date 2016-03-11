#ifndef _STK_COMCLIENT
#define _STK_COMCLIENT

#include <string>
#include <vector>

#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>

#include "types.h"
#include "socket_t.h"

namespace st3{

  /*! client side specifics */
  namespace client{
    
    /*! enum for client query status */
    const int query_query = 0; /*!< requery again */
    const int query_accepted = 1; /*!< the query was confirmed */
    const int query_aborted = 2; /*!< the query was aborted */
    const int query_game_complete = 4; /*!< the game is complete */

    struct data_frame{
      game_settings settings; /*!< the game settings */
      hm_t<idtype, player> players; /*!< data for players in the game */
      hm_t<combid, entity_selector::ptr> entity; /*!< graphical representations for solars, fleets and waypoints */
      std::list<combid> remove_entities;
    };
    
    template<typename T> 
    bool deserialize_object(data_frame &f, sf::Packet &p, sf::Color col, sint id);
 
    // attempt to deserialize from socket -> data
    bool deserialize(data_frame &f, socket_t *socket, sf::Color col);

    /*! send a query to the server and store the response
      @param socket socket to communicate on
      @param pq package with query
      @param[out] package wiht response
      @param[out] done query status
    */
    void query(socket_t *socket, 
	       sf::Packet &pq,
	       int &done);

    /*! load simulation frames 
      @param socket socket to communicate on
      @param[out] g vector to store loaded game data in
      @param[out] loaded reference to update number of loaded frames
    */
    void load_frames(socket_t *socket, std::vector<data_frame> &g, int &loaded, int &done, sf::Color col);
  };
};

#endif
