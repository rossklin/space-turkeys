#include "fleet.h"

st3::fleet::fleet(){
  position = point(0,0);
  radius = 0;
  update_counter = 0;
  owner = -1;
  converge = false;
}