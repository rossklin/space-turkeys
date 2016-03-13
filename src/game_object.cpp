#include <memory>

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
  g -> entity_grid -> remove(id);
}

game_object::ptr game_object::clone(){
  return clone_impl();
}

// commandable object
commandable_object::commandable_object() {}
commandable_object::~commandable_object() {}

commandable_object::ptr commandable_object::clone(){
  return dynamic_pointer_cast<commandable_object>(clone_impl());
}
