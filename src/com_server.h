#ifndef _STK_COMSERVER
#define _STK_COMSERVER

#include <string>
#include <vector>
#include <functional>

#include <SFML/Network.hpp>

#include "types.h"
#include "socket_t.h"
#include "game_settings.h"

namespace st3{
  class entity_package;

  /*! server side specifics */
  namespace server{
    struct handler_result {
      sf::Packet response;
      int status;
    };
      
    typedef std::function<handler_result(int cid, sf::Packet q)> query_handler;

    handler_result handler_switch(bool test, std::function<void(handler_result&)> on_success = 0, std::function<void(handler_result&)> on_fail = 0);

    /*! special socket functions for server */
    struct client_t : public socket_t{
      std::string name;
      handler_result receive_query(protocol_t p, query_handler f);
      bool check_protocol(protocol_t p, query_handler f);
      bool is_connected();
      void set_disconnect();
    };

    /*! structure handling a set of clients */
    struct com {
      
      hm_t<int, client_t*> clients;
      game_settings settings;
      int idc;
      int thread_com;

      com();

      void add_client(client_t *c);
      void disconnect();

      static query_handler basic_query_handler(protocol_t query, hm_t<int, sf::Packet> response);
      static query_handler basic_query_handler(protocol_t query, sf::Packet response);
      static query_handler basic_query_handler(protocol_t query, protocol_t response);
      bool cleanup_clients();
      bool check_protocol(protocol_t p, query_handler h);
      void distribute_frames(std::vector<entity_package> &g, int &frame_count);
    };
  };
};

#endif
