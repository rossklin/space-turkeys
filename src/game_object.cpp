#include <memory>
#include <iostream>

#include "game_object.h"
#include "game_data.h"
#include "utility.h"

using namespace std;
using namespace st3;

// game object
game_object::game_object(){
  remove = false;
}

game_object::~game_object(){}

void game_object::interact(game_data *g) {
  // check ship interactions
  auto itab = interaction::table();
  for (auto inter : this -> compile_interactions()){
    interaction i = itab[inter];
    list<combid> buf = g -> search_targets(id, position, this -> interaction_radius(), i.condition.owned_by(owner));
    list<combid> targets;

    // check whether the object wants to do the action now
    for (auto tid : buf) if (this -> confirm_interaction(inter, tid, g)) targets.push_back(tid);

    if (!targets.empty()) {
      interaction_info info;
      info.source = id;
      info.target = utility::uniform_sample(buf);
      info.interaction = inter;
      g -> interaction_buffer.push_back(info);
    }
  }
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

bool game_object::is_active(){
  return true;
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
