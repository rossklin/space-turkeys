#include "ship.h"
#include "research.h"
#include "solar.h"

using namespace st3;
using namespace std;

ship::ship(){

}

ship::ship(ship::class_t c, research &res){
  float r = res.field[research::r_ship];
  speed = 1 + r/10;
  vision = 50 + r;
  hp = 1 + r/10;
  interaction_radius = 10 + r/10;
  fleet_id = -1;
  ship_class = c;
  was_killed = false;

  if (c == solar::s_scout){
    speed += 1;
  }
}
