#include "game_object.hpp"

#include <iostream>
#include <memory>

#include "fleet.hpp"
#include "game_data.hpp"
#include "serialization.hpp"
#include "ship.hpp"
#include "solar.hpp"
#include "utility.hpp"
#include "waypoint.hpp"

using namespace std;
using namespace st3;

const string game_object::class_id = "game_object";
const string commandable_object::class_id = "commandable_object";
const string physical_object::class_id = "physical_object";

// game object
game_object::game_object() {
  remove = false;
}

game_object::game_object(const game_object &x) {
  *this = x;
}

void game_object::on_add(game_data *g) {}

void game_object::on_remove(game_data *g) {}

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
  return c == "game_object";
}

game_object_ptr game_object::deserialize(sf::Packet &p) {
  game_object_ptr res;
  class_t key;
  bool success = (p >> key);

  if (!success) {
    throw network_error("deserialize: failed to extract key!");
  } else if (key == ship::class_id) {
    ship_ptr buf = ship_ptr(new ship());
    success = (p >> *buf);
    res = buf;
  } else if (key == fleet::class_id) {
    fleet_ptr buf = fleet_ptr(new fleet());
    success = (p >> *buf);
    res = buf;
  } else if (key == solar::class_id) {
    solar_ptr buf = solar_ptr(new solar());
    success = (p >> *buf);
    res = buf;
  } else if (key == waypoint::class_id) {
    waypoint_ptr buf = waypoint_ptr(new waypoint());
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

bool commandable_object::isa(string c) {
  return c == class_id || game_object::isa(c);
}

// physical object
bool physical_object::is_physical() {
  return true;
}

bool physical_object::isa(string c) {
  return c == class_id || game_object::isa(c);
}
