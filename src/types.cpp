#include <sstream>
#include <iostream>
#include "types.h"

using namespace std;
using namespace st3;

// make a source symbol with type t and id i
source_t identifier::make(string t, idtype i){
  stringstream s;
  s << t << ":" << i;
  return s.str();
}

// make a source symbol with type t and string id k
source_t identifier::make(string t, source_t k){
  stringstream s;
  s << t << ":" << k;
  return s.str();
}

// get the type of source symbol s
string identifier::get_type(source_t s){
  size_t split = s.find(':');
  return s.substr(0, split);
}

// get the string id of source symbol s
source_t identifier::get_string_id(source_t s){
  size_t split = s.find(':');
  string v = s.substr(split + 1, s.length() - split - 1);
  return (source_t)v;
}

// get the id of source symbol s
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

// get the owner id of waypoint symbol string id v
idtype identifier::get_waypoint_owner(source_t v){
  size_t split = v.find('#');
  string x = v.substr(0, split);
  try{
    return stoi(x);
  }catch(...){
    cout << "get wp onwer: invalid wp id: " << v << endl;
    exit(-1);
  }
}

combid identifier::make_waypoint_id(idtype owner, idtype id){
  return identifier::make(identifier::waypoint, to_string(owner) + "#" + to_string(id));
}
