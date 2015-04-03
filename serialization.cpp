#include <iostream>
#include "serialization.h"

using namespace std;
using namespace st3;

// ****************************************
// GENERAL TEMPLATE STREAM OPS
// ****************************************

// packet stream ops for hm_t
template<typename T>
sf::Packet& st3::operator <<(sf::Packet& packet, const hm_t<idtype, T> &g){
  bool res = true;
  res &= (bool)(packet << (sint)g.size());
  for (auto i = g.begin(); i != g.end() && res; i++){
    res &= (bool)(packet << i -> first << i -> second);
  }
  
  return packet;
}

template<typename T>
sf::Packet& st3::operator >>(sf::Packet& packet, hm_t<idtype, T> &g){
  sint n;
  bool res = true;
  res &= (bool)(packet >> n);
  g.clear();
  for (sint i = 0; i < n && res; i++){
    idtype k;
    T v;
    res &= (bool)(packet >> k >> v);
    g[k] = v;
  }

  return packet;
}

// packet stream ops for list-like container
template<typename T>
sf::Packet& st3::operator <<(sf::Packet& packet, const list<T> &container){
  bool res = true;
  sint n = container.size();
  cout << "packet << list: n = " << n << endl;
  res &= (bool)(packet << n);
  for (auto i = container.begin(); i != container.end() && res; i++){
    res &= (bool)(packet << *i);
  }
  
  return packet;
}

template<typename T>
sf::Packet& st3::operator >>(sf::Packet& packet, list<T> &container){
  sint n;
  bool res = true;

  container.clear();
  res &= (bool)(packet >> n);
  cout << "packte >> list: n = " << n << endl;
  for (sint i = 0; i < n && res; i++){
    T v;
    res &= (bool)(packet >> v);
    container.push_back(v);
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
    << g.position
    << g.angle
    << g.speed
    << g.owner
    << g.hp
    << g.was_killed;
}

sf::Packet& st3::operator >>(sf::Packet& packet, ship &g){
  return packet 
    >> g.position
    >> g.angle
    >> g.speed
    >> g.owner
    >> g.hp
    >> g.was_killed;
}

// solar
sf::Packet& st3::operator <<(sf::Packet& packet, const solar &g){
  return packet << g.owner << g.position << g.radius;
}

sf::Packet& st3::operator >>(sf::Packet& packet, solar &g){
  return packet >> g.owner >> g.position >> g.radius;
}

// choice
sf::Packet& st3::operator <<(sf::Packet& packet, const choice &c){
  return packet << c.upgrades << c.fleet_commands << c.solar_commands;
}

sf::Packet& st3::operator >>(sf::Packet& packet, choice &c){
  return packet >> c.upgrades >> c.fleet_commands >> c.solar_commands;
}

// todo: upgrade_choice
sf::Packet& st3::operator <<(sf::Packet& packet, const upgrade_choice &c){
  return packet;
}

sf::Packet& st3::operator >>(sf::Packet& packet, upgrade_choice &c){
  return packet;
}

// todo: command
sf::Packet& st3::operator <<(sf::Packet& packet, const command &c){
  return packet;
}

sf::Packet& st3::operator >>(sf::Packet& packet, command &c){
  return packet;
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

template sf::Packet& st3::operator <<(sf::Packet& packet, const hm_t<idtype, solar> &g);
template sf::Packet& st3::operator >>(sf::Packet& packet, hm_t<idtype, solar> &g);

// instantiate templates for list<command> and list<upgrade_choice>
template sf::Packet& st3::operator <<(sf::Packet& packet, const list<command> &c);
template sf::Packet& st3::operator >>(sf::Packet& packet, list<command> &c);

template sf::Packet& st3::operator <<(sf::Packet& packet, const list<upgrade_choice> &c);
template sf::Packet& st3::operator >>(sf::Packet& packet, list<upgrade_choice> &c);

