#include "game_settings.hpp"

#include <set>

using namespace std;
using namespace st3;

const set<string> st3::starting_options = {"single", "voyagers", "battleships", "massive", "fighters"};

game_settings::game_settings() {
  solar_minrad = 10;
  solar_meanrad = 15;
  solar_density = 1e-4;
  fleet_default_radius = 10;
  dt = 0.25;
  space_index_ratio = 400;
  enable_extend = true;
}

client_game_settings::client_game_settings() {
  sim_sub_frames = 4;
  restart = 0;
  frames_per_round = 50;
  galaxy_radius = 600;
  num_players = 2;
  starting_fleet = "single";
}

bool client_game_settings::validate() {
  int very_large = 10000;
  if (frames_per_round < 1 || frames_per_round > very_large) return false;
  if (galaxy_radius < 10 || galaxy_radius > very_large) return false;
  if (num_players < 2 || num_players > 10) return false;
  if (!starting_options.count(starting_fleet)) return false;
  return true;
}
