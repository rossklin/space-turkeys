#include <iostream>
#include <cmath>

#include "game_data.h"

using namespace std;
using namespace st3;
 
void st3::game_data::apply_choice(choice c, sint id){
  cout << "game_data: running dummy apply choice" << endl;
}

void st3::game_data::increment(){
  cout << "game_data: running dummy increment" << endl;
  for (auto i = ships.begin(); i != ships.end(); i++){
    i -> second.position.x += i -> second.speed * cos(i -> second.angle);
    i -> second.position.y += i -> second.speed * sin(i -> second.angle);
    i -> second.angle += (rand() % 100) / (sfloat)1000;
    i -> second.was_killed |= !(rand() % 100);
  }
}

void st3::game_data::build(){
  cout << "game_data: running dummy build" << endl;

  // make 100 random ships
  for (idtype i = 0; i < 100; i++){
    ship s;
    s.position.x = rand() % 800;
    s.position.y = rand() % 600;
    s.speed = rand() % 10;
    s.angle = 2 * M_PI * (rand() % 1000) / (sfloat)1000;
    s.was_killed = false;
    ships[i] = s;
  }

  // make 10 random solars
  for (idtype i = 0; i < 10; i++){
    solar s;
    s.position.x = rand() % 800;
    s.position.y = rand() % 600;
    solars[i] = s;
  }

  settings.frames_per_round = 10;
}

// find all dead ships and remove them
void st3::game_data::cleanup(){
  hm_t<idtype, ship> buf;
  cout << "cleanup: start: " << ships.size() << " ships." << endl;
  for (auto x : ships){
    if (!x.second.was_killed) buf[x.first] = x.second;
  }
  ships.swap(buf);
  cout << "cleanup: end: " << ships.size() << " ships." << endl;
}
