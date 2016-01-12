#include <cstring>
#include "cost.h"

using namespace std;
using namespace st3;
using namespace cost;

sector_allocation<sector_cost> cost::sector_expansion;
ship_allocation<resource_base> cost::ship_build;

const vector<string> keywords::resource = {"organics", "metals", "gases"};
const vector<string> keywords::sector = {"research", "culture", "military", "mining", "expansion"};
const vector<string> keywords::ship = {"scout", "fighter", "bomber", "colonizer"};
const vector<string> keywords::turret = {"radar turret", "rocket turret"};

// basic allocation
template<typename T>
allocation<T>::setup(vector<string> x){
  T buf;
  data.clear();
  for (auto v : x) data[v] = T;
}

template<typename T>
T& allocation<T>::operator[](string v){
  if (data.count(v)){
    return data[v];
  }else{
    cout << "allocation access: invalid index: " << v << endl;
    exit(-1);
  }
}

template<typename T>
void allocation::confirm_content(vector<string> x){
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
  for (auto x : data) T += x.second;
  return T;
}

// specific allocations

template<typename T> ship_allocation<T>::ship_allocation() {setup(keywords::ship);}
template<typename T> turret_allocation<T>::turret_allocation() {setup(keywords::turret);}
template<typename T> resource_allocation<T>::resource_allocation() {setup(keywords::resource);}
template<typename T> sector_allocation<T>::sector_allocation() {setup(keywords::sector);}

// cost initializer

void st3::cost::initialize(){
  // sector costs
  
  // T research;
  sector_expansion["research"].res["organics"] = 1;
  sector_expansion["research"].res["gases"] = 1;
  sector_expansion["research"].res["metals"] = 1;
  sector_expansion["research"].water = 1;
  sector_expansion["research"].space = 1;
  sector_expansion["research"].time = 10;
  
  // T culture;
  sector_expansion["culture"].res["organics"] = 8;
  sector_expansion["culture"].res["gases"] = 4;
  sector_expansion["culture"].res["metals"] = 5;
  sector_expansion["culture"].water = 10;
  sector_expansion["culture"].space = 10;
  sector_expansion["culture"].time = 4;

  // T military;
  sector_expansion["military"].res["organics"] = 0;
  sector_expansion["military"].res["gases"] = 4;
  sector_expansion["military"].res["metals"] = 8;
  sector_expansion["military"].water = 2;
  sector_expansion["military"].space = 4;
  sector_expansion["military"].time = 6;

  // T mining;
  sector_expansion["mining"].res["organics"] = 0;
  sector_expansion["mining"].res["gases"] = 8;
  sector_expansion["mining"].res["metals"] = 4;
  sector_expansion["mining"].water = 4;
  sector_expansion["mining"].space = 4;
  sector_expansion["mining"].time = 6;

  // ship costs  
  ship_build["scout"].res["metals"] = 1;
  ship_build["scout"].res["gases"] = 1;
  ship_biuld["scout"].time = 1;

  ship_build["fighter"].res["metals"] = 2;
  ship_build["fighter"].res["gases"] = 1;
  ship_build["fighter"].time = 2;

  ship_build["bomber"].res["metals"] = 4;
  ship_build["bomber"].res["gases"] = 3;
  ship_build["bomber"].time = 4;

  ship_build["colonizer"].res["metals"] = 4;
  ship_build["colonizer"].res["gases"] = 2;
  ship_build["colonizer"].res["organics"] = 3;
  ship_build["colonizer"].time = 6;

  turret_build["radar turret"].res["metals"] = 1;
  turret_build["radar turret"].res["gases"] = 1;
  turret_build["radar turret"].time = 2;

  turret_build["rocket turret"].res["metals"] = 1;
  turret_build["rocket turret"].res["gases"] = 2;
  turret_build["rocket turret"].time = 2;

  // assure all content is initialized
  sector_expansion.confirm_content(keywords::sector);
  ship_build.confirm_content(keywords::ship);
  turret_build.confirm_content(keywords::turret);  
}
