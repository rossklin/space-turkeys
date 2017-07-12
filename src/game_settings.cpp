#include <set>
#include "game_settings.h"

using namespace std;

const set<string> st3::starting_options = {"single", "voyagers", "massive"};

st3::game_settings::game_settings(){
  frames_per_round = 150;
  galaxy_radius = 600;
  solar_minrad = 10;
  solar_meanrad = 15;
  solar_density = 1e-4;
  fleet_default_radius = 10;
  dt = 0.1;
  num_players = 2;
  starting_fleet = "single";
}

bool st3::game_settings::validate() {
  int very_large = 10000;
  if (frames_per_round < 1 || frames_per_round > very_large) return false;
  if (galaxy_radius < 10 || galaxy_radius > very_large) return false;
  if (num_players < 2 || num_players > 10) return false;
  if (!starting_options.count(starting_fleet)) return false;

  // todo: don't read basic settings from client
  return true;
}
