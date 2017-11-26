#ifndef _STK_SERIALIZATION
#define _STK_SERIALIZATION

#include <utility>

#include "research.h"
#include "development_tree.h"
#include "game_data.h"
#include "cost.h"

namespace st3{

  /* **************************************** */
  /*   CONTAINERS */
  /* **************************************** */
  
  // packet stream ops for hm_t
  template<typename ID, typename T>
  sf::Packet& operator <<(sf::Packet& packet, const hm_t<ID, T> &g){
    bool res = true;
    res &= (bool)(packet << (sint)g.size());
    for (auto i = g.begin(); i != g.end() && res; i++){
      res &= (bool)(packet << i -> first << i -> second);
    }
  
    return packet;
  };

  template<typename ID, typename T>
  sf::Packet& operator >>(sf::Packet& packet, hm_t<ID, T> &g){
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
  };
  
  // packet stream ops for list-like container
  template<typename T>
  sf::Packet& inner_iterated_insert(sf::Packet& packet, const T &container){
    bool res = true;
    sint n = container.size();
    res &= (bool)(packet << n);
    for (auto i = container.begin(); i != container.end() && res; i++){
      res &= (bool)(packet << *i);
    }
  
    return packet;
  }

  template<typename T>
  sf::Packet& inner_iterated_extract(sf::Packet& packet, T &container){
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
  sf::Packet& operator <<(sf::Packet& packet, const std::list<T> &container){return inner_iterated_insert(packet, container);}

  template<typename T>
  sf::Packet& operator >>(sf::Packet& packet, std::list<T> &container){return inner_iterated_extract(packet, container);}

  template<typename T>
  sf::Packet& operator <<(sf::Packet& packet, const std::vector<T> &container){return inner_iterated_insert(packet, container);}

  template<typename T>
  sf::Packet& operator >>(sf::Packet& packet, std::vector<T> &container){return inner_iterated_extract(packet, container);}

  template<typename T>
  sf::Packet& operator <<(sf::Packet& packet, const std::set<T> &container){return inner_iterated_insert(packet, container);}

  template<typename T>
  sf::Packet& operator >>(sf::Packet& packet, std::set<T> &container){return inner_iterated_extract(packet, container);}

  template<typename F, typename S>
  sf::Packet& operator <<(sf::Packet& packet, const std::pair<F,S> &g){
    return packet << g.first << g.second;
  }

  template<typename F, typename S>
  sf::Packet& operator >>(sf::Packet& packet, std::pair<F,S> &g){
    return packet >> g.first >> g.second;
  }

  /* **************************************** */
  /*   GAME DATA OBJECTS */
  /* **************************************** */

  sf::Packet& operator <<(sf::Packet& packet, const cost::allocation &g);
  sf::Packet& operator >>(sf::Packet& packet, cost::allocation &g);

  sf::Packet& operator <<(sf::Packet& packet, const entity_package &g);
  sf::Packet& operator >>(sf::Packet& packet, entity_package &g);

  sf::Packet& operator << (sf::Packet& packet, const terrain_object &g);
  sf::Packet& operator >> (sf::Packet& packet, terrain_object &g);

  sf::Packet& operator << (sf::Packet& packet, const commandable_object &g);
  sf::Packet& operator >> (sf::Packet& packet, commandable_object &g);

  sf::Packet& operator << (sf::Packet& packet, const game_object &g);
  sf::Packet& operator >> (sf::Packet& packet, game_object &g);

  sf::Packet& operator <<(sf::Packet& packet, const game_settings &g);
  sf::Packet& operator >>(sf::Packet& packet, game_settings &g);
  
  sf::Packet& operator <<(sf::Packet& packet, const client_game_settings &g);
  sf::Packet& operator >>(sf::Packet& packet, client_game_settings &g);

  // choice structs

  /*! stream a choice into a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const choice::choice &c);

  /*! stream a choice out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, choice::choice &c);

  /*! stream a waypoint into packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const waypoint &c);

  /*! stream a waypoint out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, waypoint &c);

  /*! stream a command into packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const command &c);

  /*! stream a command out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, command &c);

  sf::Packet& operator <<(sf::Packet& packet, const target_condition &c);
  sf::Packet& operator >>(sf::Packet& packet, target_condition &c);

  sf::Packet& operator <<(sf::Packet& packet, const ssfloat_t &s);
  sf::Packet& operator >>(sf::Packet& packet, ssfloat_t &s);

  sf::Packet& operator <<(sf::Packet& packet, const ship_stats &s);
  sf::Packet& operator >>(sf::Packet& packet, ship_stats &s);

  /*! stream a ship into packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const ship &g);

  /*! stream a ship out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, ship &g);

  /*! stream a turret into packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const turret_t &g);

  /*! stream a turret out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, turret_t &g);

  sf::Packet& operator <<(sf::Packet& packet, const facility &g);
  sf::Packet& operator >>(sf::Packet& packet, facility &g);

  sf::Packet& operator <<(sf::Packet& packet, const facility_object &g);
  sf::Packet& operator >>(sf::Packet& packet, facility_object &g);

  sf::Packet& operator <<(sf::Packet& packet, const development::node &g);
  sf::Packet& operator >>(sf::Packet& packet, development::node &g);

  /*! stream a solar into packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const solar &g);

  /*! stream a solar out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, solar &g);

  /*! stream a solar choice into packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const choice::c_solar &g);

  /*! stream a solar choice out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, choice::c_solar &g);

  /*! stream a fleet into packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const fleet &g);

  /*! stream a fleet out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, fleet &g);
  
  // fleet analytics
  sf::Packet& operator <<(sf::Packet& packet, const fleet::analytics &g);
  sf::Packet& operator >>(sf::Packet& packet, fleet::analytics &g);
  
  // fleet suggestion
  sf::Packet& operator <<(sf::Packet& packet, const fleet::suggestion &g);
  sf::Packet& operator >>(sf::Packet& packet, fleet::suggestion &g);

  /*! stream a point into packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const point &g);

  /*! stream a point out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, point &g);

  /*! stream a player into a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const player &g);

  /*! stream a player out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, player &g);

  sf::Packet& operator <<(sf::Packet& packet, const animation_data &g);
  sf::Packet& operator >>(sf::Packet& packet, animation_data &g);

  sf::Packet& operator <<(sf::Packet& packet, const animation_tracker_info &g);
  sf::Packet& operator >>(sf::Packet& packet, animation_tracker_info &g);

  /*! stream a research into a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const research::data &g);

  /*! stream a research out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, research::data &g);

  /*! stream a research tech into a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator <<(sf::Packet& packet, const research::tech &g);

  /*! stream a research tech out of a packet
    @param packet the packet
    @param g the object to stream
    @return reference to the resulting packet
  */
  sf::Packet& operator >>(sf::Packet& packet, research::tech &g);
};
#endif
