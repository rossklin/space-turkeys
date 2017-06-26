#include <memory>
#include <iostream>

#include "game_object.h"
#include "game_data.h"
#include "utility.h"

using namespace std;
using namespace st3;

const string commandable_object::class_id = "commandable_object";
const string physical_object::class_id = "physical_object";

// game object
game_object::game_object(){
  remove = false;
}

game_object::game_object(const game_object &x) {
  *this = x;
}

void game_object::on_add(game_data *g){
  if (this -> is_active()) g -> entity_grid -> insert(id, position);
}

void game_object::on_remove(game_data *g){
  g -> entity_grid -> remove(id);
}

bool game_object::is_commandable(){
  return false;
}

bool game_object::is_physical() {
  return false;
}

bool game_object::is_active(){
  return true;
}

bool game_object::isa(string c){
  return identifier::get_type(id) == c;
}

// commandable object
bool commandable_object::is_commandable() {
  return true;
}

// physical object
bool physical_object::is_physical() {
  return true;
}
