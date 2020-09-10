#include "serialization.hpp"

#include <iostream>
#include <set>

#include "fleet.hpp"
#include "game_base_data.hpp"
#include "interaction.hpp"
#include "ship.hpp"
#include "solar.hpp"
#include "utility.hpp"
#include "waypoint.hpp"

using namespace std;
using namespace st3;

// packet stream ops for cost::allocation
sf::Packet& st3::operator<<(sf::Packet& packet, const cost::allocation& g) {
  return packet << g.data;
}

sf::Packet& st3::operator>>(sf::Packet& packet, cost::allocation& g) {
  return packet >> g.data;
}

// entity_package
sf::Packet& st3::operator<<(sf::Packet& packet, const game_base_data& g) {
  sint n = g.entity.size();
  packet << g.idc << g.players << g.settings << g.remove_entities << g.terrain << g.discovered_universe << n;
  // polymorphic serialization
  for (auto x : g.all_entities<game_object>()) x->serialize(packet);
  return packet;
}

sf::Packet& st3::operator>>(sf::Packet& packet, game_base_data& g) {
  sint n;
  packet >> g.idc >> g.players >> g.settings >> g.remove_entities >> g.terrain >> g.discovered_universe >> n;
  // polymorphic deserialization
  g.clear_entities();
  for (int i = 0; i < n; i++) {
    game_object_ptr x = game_object::deserialize(packet);
    if (x) {
      g.add_entity(x);
    } else {
      throw network_error("entity_package::load: failed to deserialize game object!");
    }
  }
  return packet;
}

sf::Packet& st3::operator<<(sf::Packet& packet, const terrain_object& g) {
  return packet << g.type << g.center << g.border;
}

sf::Packet& st3::operator>>(sf::Packet& packet, terrain_object& g) {
  return packet >> g.type >> g.center >> g.border;
}

sf::Packet& st3::operator<<(sf::Packet& packet, const commandable_object& g) {
  return packet << static_cast<const game_object&>(g);
}

sf::Packet& st3::operator>>(sf::Packet& packet, commandable_object& g) {
  return packet >> static_cast<game_object&>(g);
}

sf::Packet& st3::operator<<(sf::Packet& packet, const game_object& g) {
  return packet << g.position << g.radius << g.owner << g.id;
}

sf::Packet& st3::operator>>(sf::Packet& packet, game_object& g) {
  packet >> g.position >> g.radius >> g.owner >> g.id;
  return packet;
}

// game_settings
sf::Packet& st3::operator<<(sf::Packet& packet, const game_settings& g) {
  return packet
         << g.clset
         << g.solar_minrad
         << g.solar_meanrad
         << g.solar_density        /*!< solars per space unit */
         << g.fleet_default_radius /*!< default radius for fleets */
         << g.dt;
}

sf::Packet& st3::operator>>(sf::Packet& packet, game_settings& g) {
  return packet >> g.clset >> g.solar_minrad >> g.solar_meanrad >> g.solar_density >> g.fleet_default_radius /*!< default radius for fleets */
         >> g.dt;
}

// game_settings
sf::Packet& st3::operator<<(sf::Packet& packet, const client_game_settings& g) {
  return packet
         << g.restart
         << g.frames_per_round /*!< number of frames in the simulation step */
         << g.galaxy_radius
         << g.num_players
         << g.starting_fleet; /*!< game time per iteration step */
}

sf::Packet& st3::operator>>(sf::Packet& packet, client_game_settings& g) {
  return packet >> g.restart >> g.frames_per_round                /*!< number of frames in the simulation step */
         >> g.galaxy_radius >> g.num_players >> g.starting_fleet; /*!< game time per iteration step */
}

// ship::target_condition
sf::Packet& st3::operator<<(sf::Packet& packet, const target_condition& c) {
  return packet << c.what << c.alignment << c.owner;
}

sf::Packet& st3::operator>>(sf::Packet& packet, target_condition& c) {
  return packet >> c.what >> c.alignment >> c.owner;
}

// ship stats
sf::Packet& st3::operator<<(sf::Packet& packet, const ship_stats& g) {
  return packet
         << g.stats
         << g.interactions
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
sf::Packet& st3::operator>>(sf::Packet& packet, ship_stats& g) {
  return packet >> g.stats >> g.interactions >> g.upgrades >> g.depends_tech >> g.depends_facility_level >> g.build_cost >> g.build_time >> g.shape >> g.ship_class >> g.tags;
}

// ship stats
sf::Packet& st3::operator<<(sf::Packet& packet, const ssfloat_t& g) {
  return packet << g.stats;
}

// ship stats
sf::Packet& st3::operator>>(sf::Packet& packet, ssfloat_t& g) {
  return packet >> g.stats;
}

// ship
sf::Packet& st3::operator<<(sf::Packet& packet, const ship& g) {
  return packet
         << static_cast<const game_object&>(g)
         << static_cast<const ship_stats&>(g)
         << g.fleet_id
         << g.angle
         << g.thrust
         << g.velocity
         << g.load
         << g.base_stats
         << g.cargo
         << g.states
         //  << g.dynamic_data
         << g.nkills
         << g.pathing_policy
         << g.private_path;
}

sf::Packet& st3::operator>>(sf::Packet& packet, ship& g) {
  return packet >> static_cast<game_object&>(g) >> static_cast<ship_stats&>(g) >> g.fleet_id >> g.angle >> g.thrust >> g.velocity >> g.load >> g.base_stats >> g.cargo >> g.states >> g.nkills >> g.pathing_policy >> g.private_path;
}

sf::Packet& st3::operator<<(sf::Packet& packet, const development::node& g) {
  return packet
         << g.name
         << g.solar_modifier
         << g.ship_upgrades
         << g.depends_facilities
         << g.depends_techs
         << g.cost_time
         << g.level
         << g.progress;
}

sf::Packet& st3::operator>>(sf::Packet& packet, development::node& g) {
  return packet >> g.name >> g.solar_modifier >> g.ship_upgrades >> g.depends_facilities >> g.depends_techs >> g.cost_time >> g.level >> g.progress;
}

// solar
sf::Packet& st3::operator<<(sf::Packet& packet, const solar& g) {
  return packet
         << static_cast<const commandable_object&>(g)
         << g.choice_data
         << g.research_points
         << g.resources
         // << g.population
         << g.ships
         << g.development
         << g.ship_progress
         << g.hp
         << g.was_discovered
         << g.known_by;
}

sf::Packet& st3::operator>>(sf::Packet& packet, solar& g) {
  return packet >> static_cast<commandable_object&>(g) >> g.choice_data >> g.research_points >> g.resources
         // >> g.population
         >> g.ships >> g.development >> g.ship_progress >> g.hp >> g.was_discovered >> g.known_by;
}

// solar choice
sf::Packet& st3::operator<<(sf::Packet& packet, const c_solar& g) {
  return packet
         << g.building_queue
         << g.ship_queue;
}

sf::Packet& st3::operator>>(sf::Packet& packet, c_solar& g) {
  return packet >> g.building_queue >> g.ship_queue;
}

// fleet
sf::Packet& st3::operator<<(sf::Packet& packet, const fleet& g) {
  return packet
         << static_cast<const commandable_object&>(g)
         << g.stats
         << g.com
         << g.position
         << g.radius
         << g.owner
         << g.ships
         << g.heading
         << g.path;
}

sf::Packet& st3::operator>>(sf::Packet& packet, fleet& g) {
  return packet >> static_cast<commandable_object&>(g) >> g.stats >> g.com >> g.position >> g.radius >> g.owner >> g.ships >> g.heading >> g.path;
}

// fleet analytics
sf::Packet& st3::operator<<(sf::Packet& packet, const fleet::analytics& g) {
  return packet
         << g.converge
         << g.speed_limit
         << g.spread_radius
         << g.spread_density
         << g.target_position
         << g.evade_path
         << g.can_evade
         << g.vision_buf
         << g.enemies
         << g.average_ship
         << g.facing_ratio;
}

sf::Packet& st3::operator>>(sf::Packet& packet, fleet::analytics& g) {
  return packet >> g.converge >> g.speed_limit >> g.spread_radius >> g.spread_density >> g.target_position >> g.evade_path >> g.can_evade >> g.vision_buf >> g.enemies >> g.average_ship >> g.facing_ratio;
}

// // fleet suggestion
// sf::Packet& st3::operator <<(sf::Packet& packet, const fleet::suggestion &g){
//   return packet << g.id << g.p;
// }

// sf::Packet& st3::operator >>(sf::Packet& packet, fleet::suggestion &g){
//   return packet >> g.id >> g.p;
// }

// choice
sf::Packet& st3::operator<<(sf::Packet& packet, const choice& c) {
  vector<waypoint> wps = utility::range_map<vector<waypoint>>([](waypoint_ptr p) { return *p; }, utility::hm_values(c.waypoints));

  return packet << c.commands << c.solar_choices << wps << c.research;
}

sf::Packet& st3::operator>>(sf::Packet& packet, choice& c) {
  vector<waypoint> wps;
  auto& res = packet >> c.commands >> c.solar_choices >> wps >> c.research;

  if (res) {
    for (auto w : wps) c.waypoints[w.id] = waypoint_ptr(new waypoint(w));
  }

  return res;
}

// waypoint
sf::Packet& st3::operator<<(sf::Packet& packet, const waypoint& c) {
  return packet
         << static_cast<const game_object&>(c)
         << c.pending_commands
         << c.position;
}

sf::Packet& st3::operator>>(sf::Packet& packet, waypoint& c) {
  return packet >> static_cast<game_object&>(c) >> c.pending_commands >> c.position;
}

// command
sf::Packet& st3::operator<<(sf::Packet& packet, const command& c) {
  return packet << c.source << c.origin << c.target << c.action << c.policy << c.ship_class << c.ships;
}

sf::Packet& st3::operator>>(sf::Packet& packet, command& c) {
  return packet >> c.source >> c.origin >> c.target >> c.action >> c.policy >> c.ship_class >> c.ships;
}

// point
sf::Packet& st3::operator<<(sf::Packet& packet, const point& c) {
  return packet << c.x << c.y;
}

sf::Packet& st3::operator>>(sf::Packet& packet, point& c) {
  return packet >> c.x >> c.y;
}

// player
sf::Packet& st3::operator<<(sf::Packet& packet, const player& c) {
  return packet << c.name << c.color << c.research_level << c.animations << c.log;
}

sf::Packet& st3::operator>>(sf::Packet& packet, player& c) {
  return packet >> c.name >> c.color >> c.research_level >> c.animations >> c.log;
}

// animation_data
sf::Packet& st3::operator<<(sf::Packet& packet, const animation_data& c) {
  return packet << c.t1 << c.t2 << c.delay << c.magnitude << c.radius << c.color << c.cat << c.text;
}

sf::Packet& st3::operator>>(sf::Packet& packet, animation_data& c) {
  return packet >> c.t1 >> c.t2 >> c.delay >> c.magnitude >> c.radius >> c.color >> c.cat >> c.text;
}

// animation_tracker_info
sf::Packet& st3::operator<<(sf::Packet& packet, const animation_tracker_info& c) {
  return packet << c.eid << c.p << c.v;
}

sf::Packet& st3::operator>>(sf::Packet& packet, animation_tracker_info& c) {
  return packet >> c.eid >> c.p >> c.v;
}

// research
sf::Packet& st3::operator<<(sf::Packet& packet, const research::data& c) {
  return packet << c.tech_map << c.facility_level << c.researching;
}

sf::Packet& st3::operator>>(sf::Packet& packet, research::data& c) {
  return packet >> c.tech_map >> c.facility_level >> c.researching;
}

sf::Packet& st3::operator<<(sf::Packet& packet, const research::tech& c) {
  return packet
         << static_cast<const development::node&>(c)
         << c.increase_fleets
         << c.increase_ships_per_fleet
         << c.order_modifier;
}

sf::Packet& st3::operator>>(sf::Packet& packet, research::tech& c) {
  return packet >> static_cast<development::node&>(c) >> c.increase_fleets >> c.increase_ships_per_fleet >> c.order_modifier;
}
