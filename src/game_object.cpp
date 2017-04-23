#include <memory>
#include <iostream>

#include "game_object.h"
#include "game_data.h"

using namespace std;
using namespace st3;

// game object
game_object::game_object(){
  remove = false;
}

game_object::~game_object(){}

void game_object::on_add(game_data *g){
  g -> entity_grid -> insert(id, position);
}

void game_object::on_remove(game_data *g){
  cout << "game_object::on_remove: " << id << endl;
  g -> entity_grid -> remove(id);
}

bool game_object::is_commandable(){
  return false;
}

bool game_object::is_active(){
  return !remove;
}

bool game_object::isa(string c){
  return identifier::get_type(id) == c;
}

game_object::ptr game_object::clone(){
  return clone_impl();
}

// commandable object
commandable_object::commandable_object() {}
commandable_object::~commandable_object() {}

bool commandable_object::is_commandable() {
  return true;
}

commandable_object::ptr commandable_object::clone(){
  return dynamic_cast<commandable_object::ptr>(clone_impl());
}
