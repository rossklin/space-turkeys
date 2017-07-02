#include "game_settings.h"

st3::game_settings::game_settings(){
  frames_per_round = 100;
  galaxy_radius = 1000;
  ship_speed = 3;
  solar_minrad = 10;
  solar_meanrad = 20;
  solar_density = 2e-4;
  fleet_default_radius = 10;
  dt = 0.1;
}
