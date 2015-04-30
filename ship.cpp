#include "ship.h"
#include "research.h"

using namespace st3;
using namespace std;

ship::ship(){

}

ship::ship(ship::class_t c, research &r){
  speed = 1;
  hp = 1;
  interaction_radius = 10;
  fleet_id = -1;
  ship_class = c;
  was_killed = false;
}
