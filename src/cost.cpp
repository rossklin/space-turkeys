#include <cstring>
#include <iostream>

#include "cost.h"
#include "ship.h"
#include "turret.h"

using namespace std;
using namespace st3;
using namespace cost;

// basic allocation
template<typename T>
void allocation<T>::setup(vector<string> x){
  T buf;
  data.clear();
  for (auto v : x) data[v] = buf;
}

template<typename T>
T& allocation<T>::operator[](const string &v){
  if (data.count(v)){
    return data[v];
  }else{
    throw runtime_error("allocation access: invalid index: " + v);
  }
}

template<typename T>
void allocation<T>::confirm_content(vector<string> x){
  for (auto v : x) {
    if (!data.count(v)) throw runtime_error("allocation: unconfirmed content: " + v);
  }
}

// countable allocation
template<typename T>
void countable_allocation<T>::setup(vector<string> x){
  T buf = 0;
  allocation<T>::data.clear();
  for (auto v : x) allocation<T>::data[v] = buf;
}

template<typename T>
T countable_allocation<T>::count(){
  T r = 0;
  for (auto x : allocation<T>::data) r += x.second;
  return r;
}

template<typename T>
void countable_allocation<T>::normalize(){
  T sum = count();
  if (sum == 0) return;  
  for (auto &x : allocation<T>::data) x.second /= sum;
}

// specific allocations

template<typename T> ship_allocation<T>::ship_allocation() {
  if (keywords::ship.empty()) throw runtime_error("ship_allocation(): no keywords!");  
  allocation<T>::setup(keywords::ship);
}

template<typename T> turret_allocation<T>::turret_allocation() {
  if (keywords::turret.empty()) throw runtime_error("turret_allocation(): no keywords!");
  allocation<T>::setup(keywords::turret);
}

template<typename T> resource_allocation<T>::resource_allocation() {
  if (keywords::resource.empty()) throw runtime_error("resource_allocation(): no keywords!");
  allocation<T>::setup(keywords::resource);
}

template<typename T>
void countable_allocation<T>::scale(float a){
  for (auto &x : allocation<T>::data) x.second *= a;
}

template<typename T>
void countable_allocation<T>::add(countable_allocation<T> a){
  for (auto &x : a.data){
    if (!allocation<T>::data.count(x.first)){
      cout << "countable_allocation::add: element mismatch: " << x.first << endl;
    }
    allocation<T>::data[x.first] += x.second;
  }
}

template<typename T> sector_allocation<T>::sector_allocation() {
  if (keywords::sector.empty()) throw runtime_error("sector_allocation(): no keywords!");
  allocation<T>::setup(keywords::sector);
}


// cost initializer
using namespace keywords;

sector_allocation<sector_cost>& cost::sector_expansion(){
  static sector_allocation<sector_cost> buf;
  static bool init = false;

  if (!init){
    init = true;
    buf.data.erase(keywords::key_expansion);

    // T research;
    buf[key_research].res[key_organics] = 1;
    buf[key_research].res[key_gases] = 1;
    buf[key_research].res[key_metals] = 1;
    buf[key_research].water = 1;
    buf[key_research].space = 1;
    buf[key_research].time = 100;
  
    // T culture;
    buf[key_culture].res[key_organics] = 8;
    buf[key_culture].res[key_gases] = 4;
    buf[key_culture].res[key_metals] = 5;
    buf[key_culture].water = 10;
    buf[key_culture].space = 10;
    buf[key_culture].time = 80;

    // T military;
    buf[key_military].res[key_organics] = 0;
    buf[key_military].res[key_gases] = 4;
    buf[key_military].res[key_metals] = 8;
    buf[key_military].water = 2;
    buf[key_military].space = 4;
    buf[key_military].time = 120;

    // T mining;
    buf[key_mining].res[key_organics] = 0;
    buf[key_mining].res[key_gases] = 8;
    buf[key_mining].res[key_metals] = 4;
    buf[key_mining].water = 4;
    buf[key_mining].space = 4;
    buf[key_mining].time = 120;

    buf.confirm_content(keywords::expansion);
  }

  return buf;
}

ship_allocation<ship_cost>& cost::ship_build(){
  static ship_allocation<ship_cost> buf;
  static bool init = false;

  if (!init){
    init = true;

    // ship costs  
    buf[key_scout].res[key_metals] = 1;
    buf[key_scout].res[key_gases] = 1;
    buf[key_scout].time = 40;

    buf[key_fighter].res[key_metals] = 2;
    buf[key_fighter].res[key_gases] = 1;
    buf[key_fighter].time = 80;

    buf[key_bomber].res[key_metals] = 4;
    buf[key_bomber].res[key_gases] = 3;
    buf[key_bomber].time = 160;

    buf[key_colonizer].res[key_metals] = 4;
    buf[key_colonizer].res[key_gases] = 2;
    buf[key_colonizer].res[key_organics] = 3;
    buf[key_colonizer].time = 240;

    buf[key_freighter].res[key_metals] = 4;
    buf[key_freighter].res[key_gases] = 2;
    buf[key_freighter].res[key_organics] = 3;
    buf[key_freighter].time = 240;
  }

  return buf;
}

float cost::expansion_multiplier(float level){
  return pow(2, floor(level));
}

// template instantiations
template struct allocation<resource_data>;
template struct allocation<st3::ship>;
template struct allocation<st3::turret>;
template struct resource_allocation<sfloat>;
template struct resource_allocation<resource_data>;
template struct ship_allocation<sfloat>;
template struct ship_allocation<st3::ship>;
template struct turret_allocation<sfloat>;
template struct turret_allocation<st3::turret>;
template struct sector_allocation<sfloat>;
template struct countable_allocation<sfloat>;
