#include <iostream>
#include "game_data.h"

using namespace std;
using namespace st3;
 
void st3::game_data::apply_choice(choice c, sint id){
  cout << "game_data: running dummy apply choice" << endl;
}

void st3::game_data::increment(){
  cout << "game_data: running dummy increment" << endl;
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
    ships[i] = s;
  }

  // make 10 random solars
  for (idtype i = 0; i < 10; i++){
    solar s;
    s.position.x = rand() % 800;
    s.position.y = rand() % 600;
    solars[i] = s;
  }

  settings.frames_per_round = 100;
}

// find all dead ships and remove them
void st3::game_data::cleanup(){
  for (auto i = ships.begin(); i != ships.end(); i++){
    if (i -> second.was_killed){
      ships.erase(i);
    }
  }
}
