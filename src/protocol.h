#ifndef _STK_PROTOCOL
#define _STK_PROTOCOL

#include "types.h"

namespace st3{
  /*! constants used in client server communication */
  namespace protocol{

    /* **************************************** */
    /* QUERIES */
    /* **************************************** */
    
    const sint game_round = 0; /*!< query whether to start the game round */
    const sint choice = 1; /*!< query whether to send the choice */
    const sint frame = 2; /*!< query to get a simulation frame */
    const sint connect = 3; /*!< query to connect */
    const sint leave = 4;

    /* **************************************** */
    /* RESPONSES */
    /* **************************************** */
    
    const sint confirm = 1000; /*!< confirm a query */
    const sint complete = 1001; /*!< tell the client the game is complete */
    const sint invalid = 1002; /*!< tell the client the query is invalid */
  };
};
#endif
