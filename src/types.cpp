#include <sstream>
#include <iostream>
#include <mutex>

#include "types.h"
#include "waypoint.h"

using namespace std;
using namespace st3;

bool server::silent_mode = false;

classified_error::classified_error(string v, string s) : runtime_error(v) {
  severity = s;
}

network_error::network_error(string v, string s) : classified_error(v, s) {}

void server::output(string v, bool force) {
  static mutex m;

  if (silent_mode) return;
  
#ifdef DEBUG
  force = true;
#endif

  if (force) {
    m.lock();
    cout << v << endl;
    m.unlock();
  }
}

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
  keywords::key_development,
  keywords::key_medicine,
  keywords::key_ecology
};

const vector<string> keywords::governor = {
  keywords::key_research,
  keywords::key_culture,
  keywords::key_military,
  keywords::key_mining,
  keywords::key_development
};

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
    throw classified_error("get multid owner: invalid id from " + v + ": " + x);
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

id_pair::id_pair(combid x, combid y) {
  a = x;
  b = y;
}

bool st3::operator < (const id_pair &x, const id_pair &y) {
  return hash<string>{}(x.a)^hash<string>{}(x.b) < hash<string>{}(y.a)^hash<string>{}(y.b);
}
