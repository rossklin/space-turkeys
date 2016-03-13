#include "interaction.h"

using namespace std;
using namespace st3;

const class_t target_condition::no_target = "no target";

// target condition
target_condition::target_condition(){}

target_condition::target_condition(sint a, class_t w) : alignment(a), what(w) {}

target_condition::target_condition(idtype o, sint a, class_t w) : owner(o), alignment(a), what(w) {}

target_condition target_condition::owned_by(idtype o){
  target_condition t = *this;
  t.owner = o;
  return t;
}

bool target_condition::requires_target(){
  return what != no_target;
}

// interaction
bool interaction::valid(target_condition c, game_object::ptr p){
  bool type_match = identifier::get_type(p -> id) == c.what;
  sint aligned = 0;
  if (c.owner == p -> owner) {
    aligned = target_condition::owned;
  }else if (p -> owner == game_object::neutral_owner) {
    aligned = target_condition::neutral;
  }else {
    aligned = target_condition::enemy;
  }
  bool align_match = aligned & c.alignment;

  return type_match && align_match;
}
