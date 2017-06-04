#include <iostream>
#include <set>

#include "serialization.h"
#include "utility.h"

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
sf::Packet& st3::operator <<(sf::Packet& packet, const cost::allocation &g){
  return packet << g.data;
}

sf::Packet& st3::operator >>(sf::Packet& packet, cost::allocation &g){
  return packet >> g.data;
}

template<typename F, typename S>
sf::Packet& st3::operator <<(sf::Packet& packet, const pair<F,S> &g){
  return packet << g.first << g.second;
}

template<typename F, typename S>
sf::Packet& st3::operator >>(sf::Packet& packet, pair<F,S> &g){
  return packet >> g.first >> g.second;
}

// instantiation to support external extraction calls in com_client
template sf::Packet& st3::operator >>(sf::Packet& packet, hm_t<int, player> &g);
template sf::Packet& st3::operator >>(sf::Packet& packet, list<combid> &container);

// ****************************************
// SPECIFIC STRUCT STREAM OPS
// ****************************************

// entity_package
sf::Packet& st3::operator <<(sf::Packet& packet, const entity_package &g){
  sint n = g.entity.size();
  packet << g.players << g.settings << g.remove_entities << n;
  // polymorphic serialization
  for (auto x : g.entity) x.second -> serialize(packet);
  return packet;
}

sf::Packet& st3::operator << (sf::Packet& packet, const commandable_object &g){
  return packet << static_cast<const game_object&>(g);
}

sf::Packet& st3::operator >> (sf::Packet& packet, commandable_object &g){
  return packet >> static_cast<game_object&>(g);
}

sf::Packet& st3::operator << (sf::Packet& packet, const game_object &g){
  return packet << g.position << g.radius << g.owner << g.id;
}

sf::Packet& st3::operator >> (sf::Packet& packet, game_object &g){
  packet >> g.position >> g.radius >> g.owner >> g.id;
  return packet;
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
sf::Packet& st3::operator <<(sf::Packet& packet, const ship_stats &g){
  return packet
    << g.stats
    << g.upgrades
    << g.depends_tech
    << g.depends_facility_level
    << g.build_cost
    << g.build_time
    << g.shape
    << g.ship_class
    << g.tags;
}

// ship stats
sf::Packet& st3::operator >>(sf::Packet& packet, ship_stats &g){
  return packet 
    >> g.stats
    >> g.upgrades
    >> g.depends_tech
    >> g.depends_facility_level
    >> g.build_cost
    >> g.build_time
    >> g.shape
    >> g.ship_class
    >> g.tags;
}


// ship stats
sf::Packet& st3::operator <<(sf::Packet& packet, const ssfloat_t &g){
  return packet << g.stats;
}

// ship stats
sf::Packet& st3::operator >>(sf::Packet& packet, ssfloat_t &g){
  return packet >> g.stats;
}

// ship
sf::Packet& st3::operator <<(sf::Packet& packet, const ship &g){
  return packet
    << static_cast<const game_object &> (g)
    << static_cast<const ship_stats &> (g)
    << g.fleet_id
    << g.angle
    << g.load
    << g.base_stats
    << g.cargo
    << g.is_landed;
}

sf::Packet& st3::operator >>(sf::Packet& packet, ship &g){
  return packet
    >> static_cast<game_object &> (g)
    >> static_cast<ship_stats &> (g)
    >> g.fleet_id
    >> g.angle
    >> g.load
    >> g.base_stats
    >> g.cargo
    >> g.is_landed;
}

// turret
sf::Packet& st3::operator <<(sf::Packet& packet, const turret_t &g){
  return packet 
    << g.range
    << g.damage
    << g.accuracy
    << g.load;
}

sf::Packet& st3::operator >>(sf::Packet& packet, turret_t &g){
  return packet 
    >> g.range
    >> g.damage
    >> g.accuracy
    >> g.load;
}

sf::Packet& st3::operator <<(sf::Packet &packet, const development::node &g) {
  return packet
    << g.name
    << g.sector_boost
    << g.ship_upgrades
    << g.depends_facilities
    << g.depends_techs
    << g.cost_time;
}

sf::Packet& st3::operator >>(sf::Packet &packet, development::node &g) {
  return packet
    >> g.name
    >> g.sector_boost
    >> g.ship_upgrades
    >> g.depends_facilities
    >> g.depends_techs
    >> g.cost_time;
}

sf::Packet& st3::operator <<(sf::Packet& packet, const facility &g){
  return packet
    << static_cast<const development::node&>(g)
    << g.vision
    << g.base_hp
    << g.shield
    << g.is_turret
    << g.turret
    << g.cost_resources
    << g.water_usage
    << g.space_usage
    << g.water_provided
    << g.space_provided;
}

sf::Packet& st3::operator >>(sf::Packet& packet, facility &g){
  return packet
    >> static_cast<development::node&>(g)
    >> g.vision
    >> g.base_hp
    >> g.shield
    >> g.is_turret
    >> g.turret
    >> g.cost_resources
    >> g.water_usage
    >> g.space_usage
    >> g.water_provided
    >> g.space_provided;
}

sf::Packet& st3::operator <<(sf::Packet& packet, const facility_object &g){
  return packet
    << static_cast<const facility&> (g)
    << g.hp
    << g.level;
}

sf::Packet& st3::operator >>(sf::Packet& packet, facility_object &g){
  return packet
    >> static_cast<facility&> (g)
    >> g.hp
    >> g.level;
}

// solar
sf::Packet& st3::operator <<(sf::Packet& packet, const solar &g){  
  return packet
    << static_cast<const commandable_object &> (g)
    << g.choice_data
    << g.fleet_growth
    << g.research_points
    << g.water
    << g.space
    << g.ecology
    << g.available_resource
    << g.resource_storage
    << g.population
    << g.happiness
    << g.ships
    << g.development
    << g.out_of_resources;
}

sf::Packet& st3::operator >>(sf::Packet& packet, solar &g){
  return packet
    >> static_cast<commandable_object &> (g)
    >> g.choice_data
    >> g.fleet_growth
    >> g.research_points
    >> g.water
    >> g.space
    >> g.ecology
    >> g.available_resource
    >> g.resource_storage
    >> g.population
    >> g.happiness
    >> g.ships
    >> g.development
    >> g.out_of_resources;
}

// solar choice
sf::Packet& st3::operator <<(sf::Packet& packet, const choice::c_solar &g){
  return packet
    << g.allocation
    << g.military
    << g.mining
    << g.development;
}

sf::Packet& st3::operator >>(sf::Packet& packet, choice::c_solar &g){
  return packet
    >> g.allocation
    >> g.military
    >> g.mining
    >> g.development;
}

// fleet
sf::Packet& st3::operator <<(sf::Packet& packet, const fleet &g){
  return packet
    << static_cast<const commandable_object &> (g)
    << g.stats
    << g.com
    << g.position
    << g.radius
    << g.owner
    << g.ships;
}

sf::Packet& st3::operator >>(sf::Packet& packet, fleet &g){
  return packet
    >> static_cast<commandable_object &> (g)
    >> g.stats
    >> g.com
    >> g.position
    >> g.radius
    >> g.owner
    >> g.ships;
}

// fleet analytics
sf::Packet& st3::operator <<(sf::Packet& packet, const fleet::analytics &g){
  return packet
    << g.converge
    << g.speed_limit
    << g.spread_radius
    << g.spread_density
    << g.target_position
    << g.path
    << g.can_evade
    << g.vision_buf
    << g.enemies
    << g.average_ship;
}

sf::Packet& st3::operator >>(sf::Packet& packet, fleet::analytics &g){
  return packet
    >> g.converge
    >> g.speed_limit
    >> g.spread_radius
    >> g.spread_density
    >> g.target_position
    >> g.path
    >> g.can_evade
    >> g.vision_buf
    >> g.enemies
    >> g.average_ship;
}

// choice
sf::Packet& st3::operator <<(sf::Packet& packet, const choice::choice &c){
  return packet << c.commands << c.solar_choices << c.waypoints << c.fleets << c.research;
}

sf::Packet& st3::operator >>(sf::Packet& packet, choice::choice &c){
  return packet >> c.commands >> c.solar_choices >> c.waypoints >> c.fleets >> c.research;
}

// waypoint
sf::Packet& st3::operator <<(sf::Packet& packet, const waypoint &c){
  return packet
    << static_cast<const game_object &> (c)
    << c.pending_commands
    << c.position;
}

sf::Packet& st3::operator >>(sf::Packet& packet, waypoint &c){
  return packet
    >> static_cast<game_object &> (c)
    >> c.pending_commands
    >> c.position;
}

// command
sf::Packet& st3::operator <<(sf::Packet& packet, const command &c){
  return packet << c.source << c.origin << c.target << c.action << c.policy << c.ships;
}

sf::Packet& st3::operator >>(sf::Packet& packet, command &c){
  return packet >> c.source >> c.origin >> c.target >> c.action >> c.policy >> c.ships;
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
  return packet << c.name << c.color << c.research_level << c.animations << c.log;
}

sf::Packet& st3::operator >>(sf::Packet& packet, player &c){
  return packet >> c.name >> c.color >> c.research_level >> c.animations >> c.log;
}

// animation_data
sf::Packet& st3::operator <<(sf::Packet& packet, const animation_data &c){
  return packet << c.p1 << c.p2 << c.v << c.magnitude << c.color << c.cat;
}

sf::Packet& st3::operator >>(sf::Packet& packet, animation_data &c){
  return packet >> c.p1 >> c.p2 >> c.v >> c.magnitude >> c.color >> c.cat;
}

// research
sf::Packet& st3::operator <<(sf::Packet& packet, const research::data &c){
  return packet << c.tech_map << c.facility_level << c.researching;
}

sf::Packet& st3::operator >>(sf::Packet& packet, research::data &c){
  return packet >> c.tech_map >> c.facility_level >> c.researching;
}

sf::Packet& st3::operator <<(sf::Packet& packet, const research::tech &c){
  return packet << static_cast<const development::node &>(c);
}

sf::Packet& st3::operator >>(sf::Packet& packet, research::tech &c){
  return packet >> static_cast<development::node &>(c);
}
