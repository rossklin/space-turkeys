#include <sstream>
#include "types.h"

using namespace std;
using namespace st3;

string identifier::make(string t, idtype i){
  stringstream s;
  s << t << ":" << i;
  return s.str();
}

string identifier::make(string t, st3::point p){
  stringstream s;
  s << t << ":" << p.x << "," << p.y;
  return s.str();
}

// string identifier::get_type(string s){

// }

// idtype identifier::get_id(string s){

// }

// point identifier::get_point(string s){

// }
