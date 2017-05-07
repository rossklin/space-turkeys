#include <sstream>
#include <iostream>

#include "types.h"
#include "waypoint.h"

using namespace std;
using namespace st3;

// make a source symbol with type t and id i
combid identifier::make(class_t t, idtype i){
  stringstream s;
  s << t << ":" << i;
  return s.str();
}

// make a source symbol with type t and string id k
combid identifier::make(class_t t, string k){
  stringstream s;
  s << t << ":" << k;
  return s.str();
}

// get the type of source symbol s
class_t identifier::get_type(combid s){
  size_t split = s.find(':');
  return s.substr(0, split);
}

// get the owner id of waypoint symbol string id v
idtype identifier::get_multid_owner(combid v){
  string x = get_multid_owner_symbol(v);
  try{
    return stoi(x);
  }catch(...){
    throw runtime_error("get multid owner: invalid id from " + v + ": " + x);
  }
}

// get the owner id of waypoint symbol string id v
string identifier::get_multid_owner_symbol(combid v){
  size_t split1 = v.find(':');
  size_t split2 = v.find('#');
  return v.substr(split1 + 1, split2 - split1 - 1);
}

combid identifier::make_waypoint_id(idtype owner, idtype id){
  return identifier::make(waypoint::class_id, to_string(owner) + "#" + to_string(id));
}
