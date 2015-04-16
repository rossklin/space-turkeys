#include "game_settings.h"

st3::game_settings::game_settings(){
  frames_per_round = 100;
  width = 400;
  height = 400;
  ship_speed = 3;
  solar_minrad = 5;
  solar_maxrad = 20;
  num_solars = 20;
  fleet_default_radius = 10;
}
