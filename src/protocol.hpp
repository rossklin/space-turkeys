#pragma once

#include "types.hpp"

namespace st3 {
/*! constants used in client server communication */
namespace protocol {

/* **************************************** */
/* MENU QUERIES */
/* **************************************** */

const sint get_status = 100;
const sint create_game = 101;
const sint join_game = 102;
const sint disconnect = 103;

/* **************************************** */
/* GAME QUERIES */
/* **************************************** */

const sint game_round = 0; /*!< query whether to start the game round */
const sint choice = 1;     /*!< query whether to send the choice */
const sint frame = 2;      /*!< query to get a simulation frame */
const sint connect = 3;    /*!< query to connect */
const sint leave = 4;
const sint load_init = 5;
const sint id = 6;
const sint any = 7;

/* **************************************** */
/* RESPONSES */
/* **************************************** */

const sint confirm = 1000;  /*!< confirm a query */
const sint complete = 1001; /*!< tell the client the game is complete */
const sint invalid = 1002;  /*!< tell the client the query is invalid */
const sint aborted = 1003;  /*!< tell the client the game was aborted */
const sint standby = 1004;
};  // namespace protocol
};  // namespace st3
