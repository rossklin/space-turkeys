#include <cstring>
#include <iostream>

#include "cost.h"
#include "ship.h"

using namespace std;
using namespace st3;
using namespace cost;

// basic allocation
void allocation::setup(vector<string> x){
  float buf = 0;
  data.clear();
  for (auto v : x) data[v] = buf;
}

sfloat& allocation::operator[](const string &v){
  if (data.count(v)){
    return data[v];
  }else{
    throw runtime_error("allocation access: invalid index: " + v);
  }
}

void allocation::confirm_content(vector<string> x){
  for (auto v : x) {
    if (!data.count(v)) throw runtime_error("allocation: unconfirmed content: " + v);
  }
}

sfloat allocation::count(){
  sfloat r = 0;
  for (auto x : data) r += x.second;
  return r;
}

void allocation::normalize() {
  sfloat sum = count();
  if (sum == 0) {
    for (auto &x : data) x.second = 1;
    sum = count();
  }
  for (auto &x : data) x.second /= sum;
}

void allocation::scale(float a){
  for (auto &x : data) x.second *= a;
}

void allocation::add(allocation a){
  for (auto &x : a.data){
    if (!data.count(x.first)) {
      throw runtime_error("countable_allocation::add: element mismatch: "  + x.first);
    }
    data[x.first] += x.second;
  }
}

// specific allocations
ship_allocation::ship_allocation() {
  if (ship::all_classes().empty()) throw runtime_error("ship_allocation(): no keywords!");
  auto buf = ship::all_classes();
  vector<string> classes(buf.begin(), buf.end());
  setup(classes);
}

resource_allocation::resource_allocation() {
  if (keywords::resource.empty()) throw runtime_error("resource_allocation(): no keywords!");
  setup(keywords::resource);
}

sector_allocation::sector_allocation() {
  if (keywords::sector.empty()) throw runtime_error("sector_allocation(): no keywords!");
  setup(keywords::sector);
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
