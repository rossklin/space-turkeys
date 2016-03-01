#include "game_object.h"
#include "game_data.h"

using namespace st3;

game_object::game_object(){
  id = id_counter++;
  remove = false;
}

void game_object::interact(game_data *g){
  auto inter = compile_interactions();
  for (auto x : inter){
    list<combid> valid_targets = g -> search_targets(position, x.radius, x.target_condition);
    for (auto a : valid_targets){
      x.perform(make_shared(this), g -> entity[a]);
    }
  }
}

void game_object::on_add(game_data *g){
  g -> entity_grid -> insert(id, position);
}

void game_object::on_remove(game_data *g){
  g -> entity_grid -> remove(id);
}
