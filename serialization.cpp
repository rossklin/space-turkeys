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
  return packet << g.players << g.ships << g.solars << g.settings;
}

sf::Packet& st3::operator >>(sf::Packet& packet, game_data &g){
  return packet >> g.players >> g.ships >> g.solars >> g.settings;
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
    // << g.was_killed
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
    // >> g.was_killed
    >> g.interaction_radius;
}

// solar
sf::Packet& st3::operator <<(sf::Packet& packet, const solar::solar &g){
  return packet
    << g.population_number
    << g.population_happy
    << g.resource
    << g.dev.industry
    << g.dev.resource
    << g.dev.new_research
    << g.ships
    << g.position
    << g.owner
    << g.radius
    << g.defense_current
    << g.defense_capacity;
}

sf::Packet& st3::operator >>(sf::Packet& packet, solar::solar &g){
  return packet
    >> g.population_number
    >> g.population_happy
    >> g.resource
    >> g.dev.industry
    >> g.dev.resource
    >> g.dev.new_research
    >> g.ships
    >> g.position
    >> g.owner
    >> g.radius
    >> g.defense_current
    >> g.defense_capacity;
}

// solar choice
sf::Packet& st3::operator <<(sf::Packet& packet, const solar::choice_t &g){
  return packet
    << g.population
    << g.dev.new_research
    << g.dev.industry
    << g.dev.resource
    << g.dev.fleet_growth;
}

sf::Packet& st3::operator >>(sf::Packet& packet, solar::choice_t &g){
  return packet
    >> g.population
    >> g.dev.new_research
    >> g.dev.industry
    >> g.dev.resource
    >> g.dev.fleet_growth;
}

// fleet
sf::Packet& st3::operator <<(sf::Packet& packet, const fleet &g){
  return packet << g.com << g.position << g.radius << g.owner << g.ships << g.converge;
}

sf::Packet& st3::operator >>(sf::Packet& packet, fleet &g){
  return packet >> g.com >> g.position >> g.radius >> g.owner >> g.ships >> g.converge;
}

// choice
sf::Packet& st3::operator <<(sf::Packet& packet, const choice &c){
  return packet << c.commands << c.solar_choices;
}

sf::Packet& st3::operator >>(sf::Packet& packet, choice &c){
  return packet >> c.commands >> c.solar_choices;
}

// command
sf::Packet& st3::operator <<(sf::Packet& packet, const command &c){
  cout << "serialization: receive command: " << endl << c.source << endl << c.target << endl;
  return packet << c.source << c.target << c.ships << c.child_commands;
}

sf::Packet& st3::operator >>(sf::Packet& packet, command &c){
  cout << "serialization: send command: " << endl << c.source << endl << c.target << endl;
  return packet >> c.source >> c.target >> c.ships >> c.child_commands;
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
  return packet << c.name << c.color;
}

sf::Packet& st3::operator >>(sf::Packet& packet, player &c){
  return packet >> c.name >> c.color;
}


// ****************************************
// TEMPLATE INSTANTIATION
// ****************************************

// instantiate templates for ship and solar
template sf::Packet& st3::operator <<(sf::Packet& packet, const hm_t<idtype, ship> &g);
template sf::Packet& st3::operator >>(sf::Packet& packet, hm_t<idtype, ship> &g);

template sf::Packet& st3::operator <<(sf::Packet& packet, const hm_t<idtype, solar::solar> &g);
template sf::Packet& st3::operator >>(sf::Packet& packet, hm_t<idtype, solar::solar> &g);

// instantiate templates for choice

template sf::Packet& st3::operator <<(sf::Packet& packet, const hm_t<source_t, list<command> > &g);
template sf::Packet& st3::operator >>(sf::Packet& packet, hm_t<source_t, list<command> > &g);

template sf::Packet& st3::operator <<(sf::Packet& packet, const list<command> &c);
template sf::Packet& st3::operator >>(sf::Packet& packet, list<command> &c);

template sf::Packet& st3::operator <<(sf::Packet& packet, const set<idtype> &c);
template sf::Packet& st3::operator >>(sf::Packet& packet, set<idtype> &c);

// template sf::Packet& st3::operator <<(sf::Packet& packet, const list<upgrade_choice> &c);
// template sf::Packet& st3::operator >>(sf::Packet& packet, list<upgrade_choice> &c);

