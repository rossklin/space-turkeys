#include <cstring>
#include <iostream>

#include "cost.h"
#include "ship.h"

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
  if (ship::all_classes().empty()) throw runtime_error("ship_allocation(): no keywords!");
  auto buf = ship::all_classes();
  vector<string> classes(buf.begin(), buf.end());
  allocation<T>::setup(classes);
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

float cost::expansion_multiplier(float level){
  return pow(2, floor(level));
}

cost::facility_cost::facility_cost() {
  water = 0;
  space = 0;
  time = 0;
}

cost::ship_cost::ship_cost() {
  time = 0;
}

// template instantiations
template struct allocation<st3::ship>;
template struct allocation<sfloat>;
template struct resource_allocation<sfloat>;
template struct ship_allocation<sfloat>;
template struct ship_allocation<st3::ship>;
template struct sector_allocation<sfloat>;
template struct countable_allocation<sfloat>;
