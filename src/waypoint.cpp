#include "waypoint.h"

using namespace std;
using namespace st3;

void waypoint::post_phase(game_data *g){  

  // trigger commands
  bool check;
  set<combid> ready_ships, arrived_ships;

  // compute landed ships
  arrived_ships.clear();
  for (auto y : g -> all_fleets()){
    if (y -> is_idle() && y -> com.target == id){
      arrived_ships += y -> ships;
    }
  }

  // evaluate commands in random order
  vector<command> buf(pending_commands.begin(), pending_commands.end());
  random_shuffle(buf.begin(), buf.end());
  cout << "relocate from " << x.first << ": command order: " << endl;
  for (auto &y : buf){
    cout << " -> " << y.target << endl;
      
    // check if all ships in command y are either landed or dead
    check = true;
    auto sbuf = y.ships() - arrived_ships();
    for (auto i : sbuf) check &= !(g -> entity.count(i));

    ready_ships = y.ships() & arrived_ships();

    if (check){
      cout << "waypoint trigger: command targeting " << y.target << "ready with " << ready_ships.size() << " ships!" << endl;
      relocate_ships(y, ready_ships, owner);
      pending_commands.remove(y);
    }
  }
}
