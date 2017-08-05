#include <iostream>
#include <mutex>

#include "game_data.h"
#include "waypoint.h"
#include "utility.h"
#include "serialization.h"

using namespace std;
using namespace st3;

const string waypoint::class_id = "waypoint";

waypoint::waypoint(idtype o){
  static int idc = 0;
  static mutex m;
  
  owner = o;
  m.lock();
  id = identifier::make(waypoint::class_id, to_string(o) + "#" + to_string(idc++));
  m.unlock();
  radius = 20;
}

waypoint::waypoint(const waypoint &x) : game_object(x) {
  *this = x;
}

void waypoint::pre_phase(game_data *g){}
void waypoint::move(game_data *g){}

float waypoint::vision(){
  return 0;
}

void waypoint::post_phase(game_data *g){
  // trigger commands
  bool check;
  set<combid> ready_ships, arrived_ships;

  // compute landed ships
  for (auto y : g -> all<fleet>()){
    if (y -> is_idle() && y -> com.target == id){
      arrived_ships += y -> ships;
    }
  }

  // evaluate commands in random order
  vector<command> buf(pending_commands.begin(), pending_commands.end());
  random_shuffle(buf.begin(), buf.end());
  for (auto &y : buf){
    // check if all ships in command y are either landed or dead
    check = true;
    set<combid> sbuf = y.ships - arrived_ships;
    for (auto i : sbuf) check &= !(g -> entity.count(i));

    if (check){
      ready_ships = y.ships & arrived_ships;
      cout << "waypoint trigger: command targeting " << y.target << " ready with " << ready_ships.size() << " ships!" << endl;
      g -> relocate_ships(y, ready_ships, owner);
      pending_commands.remove(y);
    }
  }
}

void waypoint::give_commands(list<command> c, game_data *g){
  pending_commands.insert(pending_commands.end(), c.begin(), c.end());
}

waypoint::ptr waypoint::create(idtype o){
  return ptr(new waypoint(o));
}

game_object::ptr waypoint::clone(){
  return new waypoint(*this);
}

bool waypoint::serialize(sf::Packet &p){
  return p << class_id << *this;
}

bool waypoint::isa(string c) {
  return c == waypoint::class_id || c == commandable_object::class_id;
}
