#include <iostream>
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
  stats.resize(sskey::key::count);
}

template<typename T>
modifiable_ship_stats<T>::modifiable_ship_stats(const modifiable_ship_stats &s) {
  stats = s.stats;
}

template<typename T>
typename sskey::key modifiable_ship_stats<T>::lookup_key(string name) {
  static hm_t<string, sskey::key> map;
  static bool init = false;

  if (!init) {
    map["speed"] = sskey::key::speed;
    map["hp"] = sskey::key::hp;
    map["mass"] = sskey::key::mass;
    map["accuracy"] = sskey::key::accuracy;
    map["evasion"] = sskey::key::evasion;
    map["ship damage"] = sskey::key::ship_damage;
    map["solar damage"] = sskey::key::solar_damage;
    map["interaction radius"] = sskey::key::interaction_radius;
    map["vision range"] = sskey::key::vision_range;
    map["load time"] = sskey::key::load_time;
    map["cargo capacity"] = sskey::key::cargo_capacity;
    map["depends facility level"] = sskey::key::depends_facility_level;
    map["build time"] = sskey::key::build_time;
    map["regeneration"] = sskey::key::regeneration;
    map["shield"] = sskey::key::shield;
    map["detection"] = sskey::key::detection;
    map["stealth"] = sskey::key::stealth;
    map["cannon flex"] = sskey::key::cannon_flex;
  }

  if (map.count(name)) {
    return map[name];
  }else{
    return sskey::key::count;
  }
}

// stats modifier
void ssmod_t::combine(const ssmod_t &b) {
  for (int i = 0; i < sskey::key::count; i++) {
    stats[i].combine(b.stats[i]);
  }
}

ssmod_t::ssmod_t() : modifiable_ship_stats<ship_stats_modifier>(){}
ssmod_t::ssmod_t(const ssmod_t &s) : modifiable_ship_stats<ship_stats_modifier>(s) {}

bool ssmod_t::parse(string name, string value) {
  sskey::key k = lookup_key(name);
  if (k == sskey::key::count) {
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
  sskey::key k = lookup_key(name);
  if (k == sskey::key::count) {
    return false;
  }else{
    stats[k] = value;
    return true;
  }
}

float ssfloat_t::get_dps() {
  return stats[sskey::key::accuracy] * stats[sskey::key::ship_damage] / stats[sskey::key::load_time];
}

float ssfloat_t::get_hp() {
  return stats[sskey::key::hp] * stats[sskey::key::evasion];
}

float ssfloat_t::get_strength() {
  return get_hp() * get_dps();
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
  for (int i = 0; i < sskey::key::count; i++) {
    stats[i] = b.stats[i].apply(stats[i]);
  }
}
