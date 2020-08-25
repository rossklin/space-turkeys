#include "game_base_data.hpp"

#include "solar.hpp"

using namespace st3;
using namespace std;

void game_base_data::add_entity(game_object_ptr e) {
  if (entity.count(e->id)) {
    throw logical_error("Attempted to add entity but it already exists!");
  }
  entity[e->id] = e;
  if (e->is_active()) entity_grid.insert(e->id, e->position);
}

bool game_base_data::entity_exists(combid i) const {
  return entity.count(i);
}

void game_base_data::remove_entity(combid i) {
  if (!entity.count(i)) {
    throw logical_error("Attempted to remove entity but it does not exist!");
  }

  entity_grid.remove(i);
  entity.erase(i);
}

void game_base_data::copy_from(const game_base_data &g) {
  if (entity.size()) throw logical_error("Attempting to assign game_data: has entities!");

  for (auto x : g.all_entities<game_object>()) add_entity(x->clone());
  idc = g.idc;
  players = g.players;
  settings = g.settings;
  remove_entities = g.remove_entities;
  terrain = g.terrain;
  evm = g.evm;
  discovered_universe = g.discovered_universe;
}

void game_base_data::clear_entities() {
  entity.clear();
  entity_grid.clear();
}

// // Max fleets = 2 + 2 * (nr of solars with developed defense)
// int game_base_data::get_max_fleets(idtype pid) const {
//   int res = 2;

//   auto sols = filtered_entities<solar>(pid);
//   for (auto sp : sols) {
//     if (sp->development.at(keywords::key_defense) > 0) res += 2;
//   }

//   return res;
// }

// // Max ships per fleet = 2 + 2 * (highest level of developed defense)
// int game_base_data::get_max_ships_per_fleet(idtype pid) const {
//   auto sols = filtered_entities<solar>(pid);
//   int max_lev = 0;
//   for (auto sp : sols) {
//     max_lev = max(max_lev, sp->development.at(keywords::key_defense));
//   }

//   return 2 + 2 * max_lev;
// }

// void game_base_data::rehash_grid() {
//   entity_grid.clear();
//   vector<game_object_ptr> obj = utility::hm_values(entity);
//   random_shuffle(obj.begin(), obj.end());
//   for (auto x : obj) {
//     if (x->is_active()) entity_grid.insert(x->id, x->position);
//   }
// }