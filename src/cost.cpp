#include <cstring>
#include <iostream>

#include "cost.h"
#include "ship.h"
#include "turret.h"

using namespace std;
using namespace st3;
using namespace cost;

const vector<string> keywords::resource = {
  keywords::key_metals,
  keywords::key_organics,
  keywords::key_gases
};

const vector<string> keywords::sector = {
  keywords::key_research,
  keywords::key_culture,
  keywords::key_military,
  keywords::key_mining,
  keywords::key_expansion
};

const vector<string> keywords::ship = {
  keywords::key_scout,
  keywords::key_fighter,
  keywords::key_bomber,
  keywords::key_colonizer
};
  
const vector<string> keywords::turret = {
  keywords::key_radar_turret,
  keywords::key_rocket_turret
};

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
    cout << "allocation access: invalid index: " << v << endl;
    exit(-1);
  }
}

template<typename T>
void allocation<T>::confirm_content(vector<string> x){
  for (auto v : x) {
    if (!data.count(v)) {
      cout << "allocation: unconfirmed content: " << v << endl;
      exit(-1);
    }
  }
}

template<typename T>
T countable_allocation<T>::count(){
  T r;
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
  if (keywords::ship.empty()){
    cout << "ship_allocation(): no keywords!" << endl;
    exit(-1);
  }
  
  allocation<T>::setup(keywords::ship);
}

template<typename T> turret_allocation<T>::turret_allocation() {
  if (keywords::turret.empty()){
    cout << "turret_allocation(): no keywords!" << endl;
    exit(-1);
  }
  
  allocation<T>::setup(keywords::turret);
}

template<typename T> resource_allocation<T>::resource_allocation() {
  if (keywords::resource.empty()){
    cout << "resource_allocation(): no keywords!" << endl;
    exit(-1);
  }
  
  allocation<T>::setup(keywords::resource);
}

template<typename T> sector_allocation<T>::sector_allocation() {
  if (keywords::sector.empty()){
    cout << "sector_allocation(): no keywords!" << endl;
    exit(-1);
  }
  
  allocation<T>::setup(keywords::sector);
}


// cost initializer
using namespace keywords;

sector_allocation<sector_cost>& cost::sector_expansion(){
  static sector_allocation<sector_cost> buf;
  static bool init = false;

  if (!init){
    init = true;

    // T research;
    buf[key_research].res[key_organics] = 1;
    buf[key_research].res[key_gases] = 1;
    buf[key_research].res[key_metals] = 1;
    buf[key_research].water = 1;
    buf[key_research].space = 1;
    buf[key_research].time = 10;
  
    // T culture;
    buf[key_culture].res[key_organics] = 8;
    buf[key_culture].res[key_gases] = 4;
    buf[key_culture].res[key_metals] = 5;
    buf[key_culture].water = 10;
    buf[key_culture].space = 10;
    buf[key_culture].time = 4;

    // T military;
    buf[key_military].res[key_organics] = 0;
    buf[key_military].res[key_gases] = 4;
    buf[key_military].res[key_metals] = 8;
    buf[key_military].water = 2;
    buf[key_military].space = 4;
    buf[key_military].time = 6;

    // T mining;
    buf[key_mining].res[key_organics] = 0;
    buf[key_mining].res[key_gases] = 8;
    buf[key_mining].res[key_metals] = 4;
    buf[key_mining].water = 4;
    buf[key_mining].space = 4;
    buf[key_mining].time = 6;
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
    buf[key_scout].time = 1;

    buf[key_fighter].res[key_metals] = 2;
    buf[key_fighter].res[key_gases] = 1;
    buf[key_fighter].time = 2;

    buf[key_bomber].res[key_metals] = 4;
    buf[key_bomber].res[key_gases] = 3;
    buf[key_bomber].time = 4;

    buf[key_colonizer].res[key_metals] = 4;
    buf[key_colonizer].res[key_gases] = 2;
    buf[key_colonizer].res[key_organics] = 3;
    buf[key_colonizer].time = 6;
  }

  return buf;
}

turret_allocation<turret_cost>& cost::turret_build(){
  static turret_allocation<turret_cost> buf;
  static bool init = false;

  if (!init){
    init = true;

    buf[key_radar_turret].res[key_metals] = 1;
    buf[key_radar_turret].res[key_gases] = 1;
    buf[key_radar_turret].time = 2;

    buf[key_rocket_turret].res[key_metals] = 1;
    buf[key_rocket_turret].res[key_gases] = 2;
    buf[key_rocket_turret].time = 2;
  }

  return buf;
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

