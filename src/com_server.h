#ifndef _STK_COMSERVER
#define _STK_COMSERVER

#include <string>
#include <vector>

#include <SFML/Network.hpp>

#include "types.h"
#include "socket_t.h"

namespace st3{
  class game_data;

  /*! server side specifics */
  namespace server{

    /*! special socket functions for server */
    struct client_t : public socket_t{
      std::string name; /*!< store the client's name */

      /*! send a packet containing protocol::invlaid to client */
      void send_invalid();

      /*! receive an expected query

	If the client does not send a protocol or the received
	protocol does not match query, this routine prints an error
	message and exits.
	
	@param query query protocol to expect
	@return whether the receive succeded
      */
      bool receive_query(protocol_t query);

      void check_protocol(protocol_t q, sf::Packet &p);

      bool is_connected();
    };

    /*! structure handling a set of clients */
    struct com{
      hm_t<int, client_t*> clients; /*!< clients */

      bool connect(int n);

      bool introduce();

      void disconnect();

      bool cleanup_clients();

      /*! receive an expected query and send a packet
	
       For each client, try to receive a specified query and then send
       a packet.

       @param query query to expect
       @param packet packet to send
      */
      bool check_protocol(protocol_t query, sf::Packet &packet);

      /*! receive an expected query and send multiple packets
	
       For each client, try to receive a specified query and then send
       a client specific packet.

       @param query query to expect
       @param packets vector with one packet per client
      */
      bool check_protocol(protocol_t query, hm_t<sint, sf::Packet> &packets);

      /*! distribute simulated game data to clients

	Receive frame queries from clients and send game_data packets
	until all clients have received the last game_data
	object. Does not send a required frame until the index is
	lower than the frame limit parameter.

	@param g vector of game_data objects to distribute
	@param frame_count frame limit parameter
      */
      void distribute_frames(std::vector<game_data> &g, int &frame_count);
    };
  };
};

#endif
