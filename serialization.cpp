#include <iostream>
#include <set>
#include "serialization.h"

using namespace std;
using namespace st3;

// ****************************************
// GENERAL TEMPLATE STREAM OPS
// ****************************************

// packet stream ops for hm_t
template<typename ID, typename T>
sf::Packet& st3::operator <<(sf::Packet& packet, const hm_t<ID, T> &g){
  bool res = true;
  res &= (bool)(packet << (sint)g.size());
  for (auto i = g.begin(); i != g.end() && res; i++){
    res &= (bool)(packet << i -> first << i -> second);
  }
  
  return packet;
}

template<typename ID, typename T>
sf::Packet& st3::operator >>(sf::Packet& packet, hm_t<ID, T> &g){
  sint n;
  bool res = true;
  res &= (bool)(packet >> n);
  g.clear();
  for (sint i = 0; i < n && res; i++){
    ID k;
    T v;
    res &= (bool)(packet >> k >> v);
    g[k] = v;
  }

  return packet;
}

// packet stream ops for list-like container
template<typename T>
sf::Packet& st3::operator <<(sf::Packet& packet, const T &container){
  bool res = true;
  sint n = container.size();
  res &= (bool)(packet << n);
  for (auto i = container.begin(); i != container.end() && res; i++){
    res &= (bool)(packet << *i);
  }
  
  return packet;
}

template<typename T>
sf::Packet& st3::operator >>(sf::Packet& packet, T &container){
  sint n;
  bool res = true;

  container.clear();
  res &= (bool)(packet >> n);
  for (sint i = 0; i < n && res; i++){
    typename T::value_type v;
    res &= (bool)(packet >> v);
    container.insert(container.end(), v);
  }

  return packet;
}

// ****************************************
// SPECIFIC STRUCT STREAM OPS
// ****************************************

// game_data
sf::Packet& st3::operator <<(sf::Packet& packet, const game_data &g){
  return packet << g.players << g.fleets << g.waypoints << g.ships << g.solars << g.settings << g.remove_entities;
}

sf::Packet& st3::operator >>(sf::Packet& packet, game_data &g){
  return packet >> g.players >> g.fleets >> g.waypoints >> g.ships >> g.solars >> g.settings >> g.remove_entities;
}

// game_settings
sf::Packet& st3::operator <<(sf::Packet& packet, const game_settings &g){
  return packet 
    << g.frames_per_round
    << g.width
    << g.height
    << g.ship_speed
    << g.solar_minrad
    << g.solar_maxrad
    << g.num_solars;
}

sf::Packet& st3::operator >>(sf::Packet& packet, game_settings &g){
  return packet
     >> g.frames_per_round
     >> g.width
     >> g.height
     >> g.ship_speed
     >> g.solar_minrad
     >> g.solar_maxrad
     >> g.num_solars;
}

// ship
sf::Packet& st3::operator <<(sf::Packet& packet, const ship &g){
  return packet 
    << g.ship_class
    << g.fleet_id
    << g.position
    << g.angle
    << g.speed
    << g.owner
    << g.hp
    << g.interaction_radius;
}

sf::Packet& st3::operator >>(sf::Packet& packet, ship &g){
  return packet 
    >> g.ship_class
    >> g.fleet_id
    >> g.position
    >> g.angle
    >> g.speed
    >> g.owner
    >> g.hp
    >> g.interaction_radius;
}

// solar
sf::Packet& st3::operator <<(sf::Packet& packet, const solar::solar &g){
  return packet
    << g.population_number
    << g.population_happy
    << g.usable_area
    << g.resource
    << g.dev.industry
    << g.dev.resource
    << g.dev.new_research
    << g.dev.fleet_growth
    << g.ships
    << g.position
    << g.owner
    << g.radius
    << g.defense_current
    << g.defense_capacity
    << g.vision;
}

sf::Packet& st3::operator >>(sf::Packet& packet, solar::solar &g){
  return packet
    >> g.population_number
    >> g.population_happy
    >> g.usable_area
    >> g.resource
    >> g.dev.industry
    >> g.dev.resource
    >> g.dev.new_research
    >> g.dev.fleet_growth
    >> g.ships
    >> g.position
    >> g.owner
    >> g.radius
    >> g.defense_current
    >> g.defense_capacity
    >> g.vision;
}

// solar choice
sf::Packet& st3::operator <<(sf::Packet& packet, const solar::choice_t &g){
  return packet << g.workers << g.sector << g.subsector;
}

sf::Packet& st3::operator >>(sf::Packet& packet, solar::choice_t &g){
  return packet >> g.workers >> g.sector >> g.subsector;
}

// fleet
sf::Packet& st3::operator <<(sf::Packet& packet, const fleet &g){
  return packet << g.com << g.position << g.radius << g.vision << g.owner << g.ships << g.converge;
}

sf::Packet& st3::operator >>(sf::Packet& packet, fleet &g){
  return packet >> g.com >> g.position >> g.radius >> g.vision >> g.owner >> g.ships >> g.converge;
}

// choice
sf::Packet& st3::operator <<(sf::Packet& packet, const choice &c){
  return packet << c.commands << c.solar_choices << c.waypoints;
}

sf::Packet& st3::operator >>(sf::Packet& packet, choice &c){
  return packet >> c.commands >> c.solar_choices >> c.waypoints;
}

// waypoint
sf::Packet& st3::operator <<(sf::Packet& packet, const waypoint &c){
  return packet << c.pending_commands << c.position;
}

sf::Packet& st3::operator >>(sf::Packet& packet, waypoint &c){
  return packet >> c.pending_commands >> c.position;
}

// command
sf::Packet& st3::operator <<(sf::Packet& packet, const command &c){
  cout << "serialization: send command: " << endl << c.source << endl << c.target << endl;
  return packet << c.source << c.target << c.ships;
}

sf::Packet& st3::operator >>(sf::Packet& packet, command &c){
  sf::Packet& res = packet >> c.source >> c.target >> c.ships;
  cout << "serialization: receive command: " << endl << c.source << endl << c.target << endl;
  return res;
}

// point
sf::Packet& st3::operator <<(sf::Packet& packet, const point &c){
  return packet << c.x << c.y;
}

sf::Packet& st3::operator >>(sf::Packet& packet, point &c){
  return packet >> c.x >> c.y;
}

// player
sf::Packet& st3::operator <<(sf::Packet& packet, const player &c){
  return packet << c.name << c.color << c.research_level;
}

sf::Packet& st3::operator >>(sf::Packet& packet, player &c){
  return packet >> c.name >> c.color >> c.research_level;
}

// research
sf::Packet& st3::operator <<(sf::Packet& packet, const research &c){
  return packet << c.field;
}

sf::Packet& st3::operator >>(sf::Packet& packet, research &c){
  return packet >> c.field;
}
