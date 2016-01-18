#include <cstring>
#include <iostream>

#include "cost.h"
#include "ship.h"
#include "turret.h"

using namespace std;
using namespace st3;
using namespace cost;

sector_allocation<sector_cost> cost::sector_expansion;
ship_allocation<ship_cost> cost::ship_build;
turret_allocation<turret_cost> cost::turret_build;

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

template<typename T> ship_allocation<T>::ship_allocation() {allocation<T>::setup(keywords::ship);}
template<typename T> turret_allocation<T>::turret_allocation() {allocation<T>::setup(keywords::turret);}
template<typename T> resource_allocation<T>::resource_allocation() {allocation<T>::setup(keywords::resource);}
template<typename T> sector_allocation<T>::sector_allocation() {allocation<T>::setup(keywords::sector);}

// cost initializer
using namespace keywords;
void st3::cost::initialize(){
  // sector costs
  
  // T research;
  sector_expansion[key_research].res[key_organics] = 1;
  sector_expansion[key_research].res[key_gases] = 1;
  sector_expansion[key_research].res[key_metals] = 1;
  sector_expansion[key_research].water = 1;
  sector_expansion[key_research].space = 1;
  sector_expansion[key_research].time = 10;
  
  // T culture;
  sector_expansion[key_culture].res[key_organics] = 8;
  sector_expansion[key_culture].res[key_gases] = 4;
  sector_expansion[key_culture].res[key_metals] = 5;
  sector_expansion[key_culture].water = 10;
  sector_expansion[key_culture].space = 10;
  sector_expansion[key_culture].time = 4;

  // T military;
  sector_expansion[key_military].res[key_organics] = 0;
  sector_expansion[key_military].res[key_gases] = 4;
  sector_expansion[key_military].res[key_metals] = 8;
  sector_expansion[key_military].water = 2;
  sector_expansion[key_military].space = 4;
  sector_expansion[key_military].time = 6;

  // T mining;
  sector_expansion[key_mining].res[key_organics] = 0;
  sector_expansion[key_mining].res[key_gases] = 8;
  sector_expansion[key_mining].res[key_metals] = 4;
  sector_expansion[key_mining].water = 4;
  sector_expansion[key_mining].space = 4;
  sector_expansion[key_mining].time = 6;

  // ship costs  
  ship_build[key_scout].res[key_metals] = 1;
  ship_build[key_scout].res[key_gases] = 1;
  ship_build[key_scout].time = 1;

  ship_build[key_fighter].res[key_metals] = 2;
  ship_build[key_fighter].res[key_gases] = 1;
  ship_build[key_fighter].time = 2;

  ship_build[key_bomber].res[key_metals] = 4;
  ship_build[key_bomber].res[key_gases] = 3;
  ship_build[key_bomber].time = 4;

  ship_build[key_colonizer].res[key_metals] = 4;
  ship_build[key_colonizer].res[key_gases] = 2;
  ship_build[key_colonizer].res[key_organics] = 3;
  ship_build[key_colonizer].time = 6;

  turret_build[key_radar_turret].res[key_metals] = 1;
  turret_build[key_radar_turret].res[key_gases] = 1;
  turret_build[key_radar_turret].time = 2;

  turret_build[key_rocket_turret].res[key_metals] = 1;
  turret_build[key_rocket_turret].res[key_gases] = 2;
  turret_build[key_rocket_turret].time = 2;

  // assure all content is initialized
  sector_expansion.confirm_content(keywords::sector);
  ship_build.confirm_content(keywords::ship);
  turret_build.confirm_content(keywords::turret);  
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

