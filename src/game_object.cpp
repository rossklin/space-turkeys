#include <iostream>
#include <memory>

#include "game_data.h"
#include "game_object.h"
#include "serialization.h"
#include "utility.h"

using namespace std;
using namespace st3;

const string commandable_object::class_id = "commandable_object";
const string physical_object::class_id = "physical_object";

// game object
game_object::game_object() {
  remove = false;
}

game_object::game_object(const game_object &x) {
  *this = x;
}

void game_object::on_add(game_data *g) {
  if (this->is_active()) g->entity_grid->insert(id, position);
}

void game_object::on_remove(game_data *g) {
  g->entity_grid->remove(id);
}

bool game_object::is_commandable() {
  return false;
}

bool game_object::is_physical() {
  return false;
}

bool game_object::is_active() {
  return true;
}

bool game_object::isa(string c) {
  return identifier::get_type(id) == c;
}

game_object::ptr game_object::deserialize(sf::Packet &p) {
  game_object::ptr res;
  class_t key;
  bool success = (p >> key);

  if (!success) {
    throw network_error("deserialize: failed to extract key!");
  } else if (key == ship::class_id) {
    ship::ptr buf = ship::ptr(new ship());
    success = (p >> *buf);
    res = buf;
  } else if (key == fleet::class_id) {
    fleet::ptr buf = fleet::ptr(new fleet());
    success = (p >> *buf);
    res = buf;
  } else if (key == solar::class_id) {
    solar::ptr buf = solar::ptr(new solar());
    success = (p >> *buf);
    res = buf;
  } else if (key == waypoint::class_id) {
    waypoint::ptr buf = waypoint::ptr(new waypoint());
    success = (p >> *buf);
    res = buf;
  } else {
    throw network_error("deserialize: key " + key + " not recognized!");
  }

  return success ? res : 0;
}

// commandable object
bool commandable_object::is_commandable() {
  return true;
}

// physical object
bool physical_object::is_physical() {
  return true;
}
