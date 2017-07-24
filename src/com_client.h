#ifndef _STK_COMCLIENT
#define _STK_COMCLIENT

#include <string>
#include <vector>

#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>

#include "types.h"
#include "socket_t.h"
#include "data_frame.h"
#include "selector.h"

namespace st3{

  /*! client side specifics */
  namespace client{

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
    void load_frames(socket_t *socket, std::vector<data_frame> &g, int &loaded, int &tc_in, int &tc_out);

    template<typename T> 
    entity_selector::ptr deserialize_object(sf::Packet &p, sint id);
    
    bool deserialize(data_frame &f, sf::Packet &p, sint id);

  };
};

#endif
