#include <iostream>

#include "game_data.h"
#include "waypoint.h"
#include "utility.h"
#include "serialization.h"

using namespace std;
using namespace st3;

const string waypoint::class_id = "waypoint";

waypoint::waypoint(){}
waypoint::waypoint(idtype o){
  static int idc = 0;
  
  owner = o;
  id = identifier::make(waypoint::class_id, to_string(o) + "#" + to_string(idc++));
  radius = 20;
}

waypoint::~waypoint(){}

void waypoint::pre_phase(game_data *g){}
void waypoint::move(game_data *g){}
void waypoint::interact(game_data *g){}

float waypoint::vision(){
  return 0;
}

void waypoint::post_phase(game_data *g){  

  // trigger commands
  bool check;
  set<combid> ready_ships, arrived_ships;

  // compute landed ships
  arrived_ships.clear();
  for (auto y : g -> all<fleet>()){
    if (y -> is_idle() && y -> com.target == id){
      arrived_ships += y -> ships;
    }
  }

  // evaluate commands in random order
  vector<command> buf(pending_commands.begin(), pending_commands.end());
  random_shuffle(buf.begin(), buf.end());
  cout << "relocate from " << id << ": command order: " << endl;
  for (auto &y : buf){
    cout << " -> " << y.target << endl;
      
    // check if all ships in command y are either landed or dead
    check = true;
    set<combid> sbuf = y.ships - arrived_ships;
    for (auto i : sbuf) check &= !(g -> entity.count(i));

    ready_ships = y.ships & arrived_ships;

    if (check){
      cout << "waypoint trigger: command targeting " << y.target << " ready with " << ready_ships.size() << " ships!" << endl;
      g -> relocate_ships(y, ready_ships, owner);
      pending_commands.remove(y);
    }
  }
}

waypoint::ptr waypoint::create(idtype o){
  return ptr(new waypoint(o));
}

waypoint::ptr waypoint::clone(){
  return dynamic_pointer_cast<waypoint>(clone_impl());
}

game_object::ptr waypoint::clone_impl(){
  return ptr(new waypoint(*this));
}

bool waypoint::serialize(sf::Packet &p){
  return p << class_id << *this;
}

void waypoint::copy_from(const waypoint &s){
  (*this) = s;
}
