#include "game_object.h"
#include "game_data.h"

using namespace st3;

game_object::game_object(){
  remove = false;
}

void game_object::on_add(game_data *g){
  g -> entity_grid -> insert(id, position);
}

void game_object::on_remove(game_data *g){
  g -> entity_grid -> remove(id);
}
