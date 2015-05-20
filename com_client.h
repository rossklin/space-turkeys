#ifndef _STK_COMCLIENT
#define _STK_COMCLIENT

#include <string>
#include <vector>

#include <SFML/Network.hpp>

#include "types.h"
#include "socket_t.h"

namespace st3{
  struct game_data;

  namespace client{
    /*! enum for client query status */
    const int query_query = 0; /*!< requery */
    const int query_accepted = 1; /*!< query complete */
    const int query_game_complete = 2; /*!< game complete */

    /*! send a query to the server and store the response
      @param socket socket to communicate on
      @param pq package with query
      @param[out] package wiht response
      @param[out] done query status
    */
    void query(socket_t socket, 
	       sf::Packet &pq,
	       sf::Packet &pr,
	       int &done);

    /*! load simulation frames 
      @param socket socket to communicate on
      @param[out] g vector to store loaded game data in
      @param[out] loaded reference to update number of loaded frames
    */
    void load_frames(socket_t socket, std::vector<game_data> &g, int &loaded);
  };
};

#endif
