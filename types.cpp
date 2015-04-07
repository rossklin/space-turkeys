#include <sstream>
#include <iostream>
#include "types.h"

using namespace std;
using namespace st3;

source_t identifier::make(string t, idtype i){
  stringstream s;
  s << t << ":" << i;
  return s.str();
}

source_t identifier::make(string t, st3::point p){
  stringstream s;
  s << t << ":" << p.x << "," << p.y;
  return s.str();
}

string identifier::get_type(source_t s){
  size_t split = s.find(':');
  return s.substr(0, split);
}

idtype identifier::get_id(source_t s){
  size_t split = s.find(':');
  string v = s.substr(split + 1, s.length() - split - 1);
  try{
    return stoi(v);
  }catch(...){
    cout << "invalid source id: " << v << endl;
    exit(-1);
  }
}

point identifier::get_point(source_t s){
  size_t split = s.find(':');
  string v = s.substr(split + 1, s.length() - split - 1);
  size_t s2 = v.find(',');
  string vx = v.substr(0, s2);
  string vy = v.substr(s2 + 1, v.length() - s2 - 1);
  st3::point p;

  try{
    p.x = stoi(vx);
    p.y = stoi(vy);
  }catch(...){
    cout << "invalid source point: " << vx << ", " << vy << endl;
    exit(-1);
  }

  return p;
}
