#pragma once

#include "types.hpp"

namespace st3 {
namespace protocol {
/*! constants used in client server communication */
enum protocol : sint {

  /* **************************************** */
  /* MENU QUERIES */
  /* **************************************** */

  get_status,
  create_game,
  join_game,
  disconnect,

  /* **************************************** */
  /* GAME QUERIES */
  /* **************************************** */

  game_round,
  choice,
  frame,
  connect,
  leave,
  load_init,
  id,
  any,

  /* **************************************** */
  /* RESPONSES */
  /* **************************************** */

  confirm,
  complete,
  invalid,
  aborted,
  standby,

  // limit
  NUM
};
};  // namespace protocol
};  // namespace st3
