#include "ship_stats.h"

using namespace std;
using namespace st3;

// modifier
ship_stats_modifier::ship_stats_modifier() {
  a = 1;
  b = 0;
}

float ship_stats_modifier::apply(float x) const {
  return a * x + b;
}

void ship_stats_modifier::combine(const ship_stats_modifier &x) {
  a *= x.a;
  b += x.b;
}

void ship_stats_modifier::parse(std::string v) {
  int idx = v.find('#');
  a = stof(v.substr(0, idx));
  b = stof(v.substr(idx + 1));
}

// base class
template<typename T>
modifiable_ship_stats<T>::modifiable_ship_stats() {
  stats.resize(key::count);
}

template<typename T>
modifiable_ship_stats<T>::modifiable_ship_stats(const modifiable_ship_stats &s) {
  stats = s.stats;
}

template<typename T>
typename modifiable_ship_stats<T>::key modifiable_ship_stats<T>::lookup_key(string name) {
  static hm_t<string, key> map;
  static bool init = false;

  if (!init) {
    map["speed"] = key::speed;
    map["hp"] = key::hp;
    map["mass"] = key::mass;
    map["accuracy"] = key::accuracy;
    map["evasion"] = key::evasion;
    map["ship damage"] = key::ship_damage;
    map["solar damage"] = key::solar_damage;
    map["interaction radius"] = key::interaction_radius;
    map["vision range"] = key::vision_range;
    map["load time"] = key::load_time;
    map["cargo capacity"] = key::cargo_capacity;
    map["depends facility level"] = key::depends_facility_level;
    map["build time"] = key::build_time;
    map["regeneration"] = key::regeneration;
    map["shield"] = key::shield;
    map["detection"] = key::detection;
    map["stealth"] = key::stealth;
  }

  if (map.count(name)) {
    return map[name];
  }else{
    return key::count;
  }
}

// stats modifier
void ssmod_t::combine(const ssmod_t &b) {
  for (int i = 0; i < key::count; i++) {
    stats[i].combine(b.stats[i]);
  }
}

ssmod_t::ssmod_t() : modifiable_ship_stats<ship_stats_modifier>(){}
ssmod_t::ssmod_t(const ssmod_t &s) : modifiable_ship_stats<ship_stats_modifier>(s) {}

bool ssmod_t::parse(string name, string value) {
  key k = lookup_key(name);
  if (k == key::count) {
    return false;
  }else{
    stats[k].parse(value);
    return true;
  }
}

// float
ssfloat_t::ssfloat_t() : modifiable_ship_stats<sfloat>(){
  for (auto &x : stats) x = 0;
}
ssfloat_t::ssfloat_t(const ssfloat_t &s) : modifiable_ship_stats<sfloat>(s) {}

bool ssfloat_t::insert(string name, float value) {
  key k = lookup_key(name);
  if (k == key::count) {
    return false;
  }else{
    stats[k] = value;
    return true;
  }
}

// ship_stats
ship_stats::ship_stats() : ssfloat_t(){
  depends_facility_level = 0;
  build_time = 0;
}

ship_stats::ship_stats(const ship_stats &s) : ssfloat_t(s) {
  ship_class = s.ship_class;
  tags = s.tags;
  upgrades = s.upgrades;
  depends_tech = s.depends_tech;
  depends_facility_level = s.depends_facility_level;
  build_cost = s.build_cost;
  build_time = s.build_time;
  shape = s.shape;
}

void ship_stats::modify_with(const ssmod_t &b) {
  for (int i = 0; i < key::count; i++) {
    stats[i] = b.stats[i].apply(stats[i]);
  }
}
