#include "game_base_data.h"

#include "game_data.h"

using namespace st3;
using namespace std;

int game_base_data::next_id(class_t x) {
  if (!idc.count(x)) idc[x] = 0;
  return idc[x]++;
}

game_object::ptr game_base_data::get_entity(combid i) const {
  if (entity.count(i)) {
    return entity.at(i);
  } else {
    throw logical_error("game_data::get_entity: not found: " + i);
  }
}

ship::ptr game_base_data::get_ship(combid i) const {
  return utility::guaranteed_cast<ship>(get_entity(i));
}

fleet::ptr game_base_data::get_fleet(combid i) const {
  return utility::guaranteed_cast<fleet>(get_entity(i));
}

solar::ptr game_base_data::get_solar(combid i) const {
  return utility::guaranteed_cast<solar>(get_entity(i));
}

waypoint::ptr game_base_data::get_waypoint(combid i) const {
  return utility::guaranteed_cast<waypoint>(get_entity(i));
}

list<game_object::ptr> game_base_data::all_owned_by(idtype id) const {
  list<game_object::ptr> res;

  for (auto p : entity) {
    if (p.second->owner == id) res.push_back(p.second);
  }

  return res;
}

// limit_to without deallocating
void game_base_data::limit_to(idtype id) {
  list<combid> remove_buf;
  for (auto i : entity) {
    bool known = i.second->isa(solar::class_id) && get_solar(i.first)->known_by.count(id);
    bool owned = i.second->owner == id;
    bool seen = evm[id].count(i.first);
    if (!(known || owned || seen)) remove_buf.push_back(i.first);
  }
  for (auto i : remove_buf) entity.erase(i);
}

void game_base_data::copy_from(const game_data &g) {
  if (entity.size()) throw logical_error("Attempting to assign game_data: has entities!");

  for (auto x : g.entity) entity[x.first] = x.second->clone();
  players = g.players;
  settings = g.settings;
  remove_entities = g.remove_entities;
  terrain = g.terrain;
  evm = g.evm;
}

void game_base_data::clear_entities() {
  for (auto x : entity) delete x.second;
  entity.clear();
}
