#include <iostream>
#include <set>

#include "serialization.h"
#include "selector.h"

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
sf::Packet& st3::inner_iterated_insert(sf::Packet& packet, const T &container){
  bool res = true;
  sint n = container.size();
  res &= (bool)(packet << n);
  for (auto i = container.begin(); i != container.end() && res; i++){
    res &= (bool)(packet << *i);
  }
  
  return packet;
}

template<typename T>
sf::Packet& st3::inner_iterated_extract(sf::Packet& packet, T &container){
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

template<typename T>
sf::Packet& st3::operator <<(sf::Packet& packet, const std::list<T> &container){return inner_iterated_insert(packet, container);}

template<typename T>
sf::Packet& st3::operator >>(sf::Packet& packet, std::list<T> &container){return inner_iterated_extract(packet, container);}

template<typename T>
sf::Packet& st3::operator <<(sf::Packet& packet, const std::vector<T> &container){return inner_iterated_insert(packet, container);}

template<typename T>
sf::Packet& st3::operator >>(sf::Packet& packet, std::vector<T> &container){return inner_iterated_extract(packet, container);}

template<typename T>
sf::Packet& st3::operator <<(sf::Packet& packet, const std::set<T> &container){return inner_iterated_insert(packet, container);}

template<typename T>
sf::Packet& st3::operator >>(sf::Packet& packet, std::set<T> &container){return inner_iterated_extract(packet, container);}

// packet stream ops for cost::allocation
template<typename T>
sf::Packet& st3::operator <<(sf::Packet& packet, const cost::allocation<T> &g){
  return packet << g.data;
}

template<typename T>
sf::Packet& st3::operator >>(sf::Packet& packet, cost::allocation<T> &g){
  return packet >> g.data;
}


// ****************************************
// SPECIFIC STRUCT STREAM OPS
// ****************************************

// game_data
sf::Packet& st3::operator <<(sf::Packet& packet, const game_data &g){
  sint n = g.entity.size();
  packet << g.players << g.settings << g.remove_entities << n;
  // polymorphic serialization
  for (auto x : g.entity) x.second -> serialize(packet);
  return packet;
}

template<typename T> 
bool st3::deserialize_object(data_frame &f, sf::Packet &p, sf::Color col, sint id){
  // assure that T is a properly setup entity selector
  static_assert(is_base_of<client::entity_selector, T>::value, "deserialize entity_selector");
  static_assert(is_base_of<game_object, typename T::base_object_t>::value, "selector base object type must inherit game_object");
  
  typename T::base_object_t s;
  if (!(p >> s)){
    cout << "deserialize object: package empty!" << endl;
    return false;
  }
  f.entity[s.id] = T::create(s, col, s.owner == id);
}

bool st3::deserialize(data_frame &f, sf::Packet &p, sf::Color col, sint id){
  sint n;
  
  if (!(p >> f.players >> f.settings >> f.remove_entities >> n)){
    cout << "deserialize: package empty!" << endl;
    return false;
  }
  
  f.entity.clear();

  // "polymorphic" deserialization
  for (int i = 0; i < n; i++){
    class_t key;
    p >> key;
    if (key == ship::class_id){
      if (!deserialize_object<client::ship_selector>(f, p, col, id)) return false;
    }else if (key == fleet::class_id){
      if (!deserialize_object<client::fleet_selector>(f, p, col, id)) return false;
    }else if (key == solar::class_id){
      if (!deserialize_object<client::solar_selector>(f, p, col, id)) return false;
    }else if (key == waypoint::class_id){
      if (!deserialize_object<client::waypoint_selector>(f, p, col, id)) return false;
    }else{
      cout << "deserialize: key " << key << " not recognized!" << endl;
      exit(-1);
    }
  }
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
    << g.num_solars
    << g.dt;
}

sf::Packet& st3::operator >>(sf::Packet& packet, game_settings &g){
  return packet
     >> g.frames_per_round
     >> g.width
     >> g.height
     >> g.ship_speed
     >> g.solar_minrad
     >> g.solar_maxrad
     >> g.num_solars
     >> g.dt;
}

// ship::target_condition
sf::Packet& st3::operator <<(sf::Packet& packet, const target_condition &c){
  return packet << c.what << c.alignment << c.owner;
}

sf::Packet& st3::operator >>(sf::Packet& packet, target_condition &c){
  return packet >> c.what >> c.alignment >> c.owner;
}

// ship stats
sf::Packet& st3::operator <<(sf::Packet& packet, const ship::stats &g){
  return packet 
    << g.speed
    << g.hp
    << g.accuracy
    << g.ship_damage
    << g.solar_damage
    << g.interaction_radius
    << g.vision
    << g.load_time;
}

// ship stats
sf::Packet& st3::operator >>(sf::Packet& packet, ship::stats &g){
  return packet 
    >> g.speed
    >> g.hp
    >> g.accuracy
    >> g.ship_damage
    >> g.solar_damage
    >> g.interaction_radius
    >> g.vision
    >> g.load_time;
}

// ship
sf::Packet& st3::operator <<(sf::Packet& packet, const ship &g){
  return packet 
    << g.ship_class
    << g.fleet_id
    << g.angle
    << g.load
    << g.base_stats
    << g.current_stats
    << g.upgrades;
}

sf::Packet& st3::operator >>(sf::Packet& packet, ship &g){
  return packet 
    >> g.ship_class
    >> g.fleet_id
    >> g.angle
    >> g.load
    >> g.base_stats
    >> g.current_stats
    >> g.upgrades;
}

// turret
sf::Packet& st3::operator <<(sf::Packet& packet, const turret &g){
  return packet 
    << g.turret_class
    << g.range
    << g.vision
    << g.damage
    << g.accuracy
    << g.load_time
    << g.load
    << g.hp;
}

sf::Packet& st3::operator >>(sf::Packet& packet, turret &g){
  return packet 
    >> g.turret_class
    >> g.range
    >> g.vision
    >> g.damage
    >> g.accuracy
    >> g.load_time
    >> g.load
    >> g.hp;
}

// solar
sf::Packet& st3::operator <<(sf::Packet& packet, const solar &g){
  return packet
    << g.fleet_growth
    << g.turret_growth
    << g.turrets
    << g.research
    << g.water
    << g.space
    << g.ecology
    << g.resource
    << g.sector
    << g.population
    << g.happiness
    << g.ships
    << g.position
    << g.owner
    << g.radius
    << g.damage_taken;
}

sf::Packet& st3::operator >>(sf::Packet& packet, solar &g){
  return packet
    >> g.fleet_growth
    >> g.turret_growth
    >> g.turrets
    >> g.research
    >> g.water
    >> g.space
    >> g.ecology
    >> g.resource
    >> g.sector
    >> g.population
    >> g.happiness
    >> g.ships
    >> g.position
    >> g.owner
    >> g.radius
    >> g.damage_taken;
}

// solar choice
sf::Packet& st3::operator <<(sf::Packet& packet, const choice::c_solar &g){
  return packet
    << g.allocation
    << g.military.c_ship
    << g.military.c_turret
    << g.mining
    << g.expansion;
}

sf::Packet& st3::operator >>(sf::Packet& packet, choice::c_solar &g){
  return packet
    >> g.allocation
    >> g.military.c_ship
    >> g.military.c_turret
    >> g.mining
    >> g.expansion;
}

// fleet
sf::Packet& st3::operator <<(sf::Packet& packet, const fleet &g){
  return packet << g.com << g.position << g.radius << g.vision_buf << g.owner << g.ships << g.converge;
}

sf::Packet& st3::operator >>(sf::Packet& packet, fleet &g){
  return packet >> g.com >> g.position >> g.radius >> g.vision_buf >> g.owner >> g.ships >> g.converge;
}

// choice
sf::Packet& st3::operator <<(sf::Packet& packet, const choice::choice &c){
  return packet << c.commands << c.solar_choices << c.waypoints << c.research;
}

sf::Packet& st3::operator >>(sf::Packet& packet, choice::choice &c){
  return packet >> c.commands >> c.solar_choices >> c.waypoints >> c.research;
}

sf::Packet& st3::operator <<(sf::Packet& packet, const choice::c_research &g){
  return packet << g.identifier;
}

sf::Packet& st3::operator >>(sf::Packet& packet, choice::c_research &g){
  return packet >> g.identifier;
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
  return packet << c.source << c.target << c.action << c.ships;
}

sf::Packet& st3::operator >>(sf::Packet& packet, command &c){
  return packet >> c.source >> c.target >> c.action >> c.ships;
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
sf::Packet& st3::operator <<(sf::Packet& packet, const research::data &c){
  return packet << c.x;
}

sf::Packet& st3::operator >>(sf::Packet& packet, research::data &c){
  return packet >> c.x;
}

sf::Packet& st3::operator <<(sf::Packet& packet, const cost::resource_data &g){
  return packet << g.available << g.storage;
}

sf::Packet& st3::operator >>(sf::Packet& packet, cost::resource_data &g){
  return packet >> g.available >> g.storage;
}
