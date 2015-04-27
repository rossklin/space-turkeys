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

source_t identifier::make(string t, source_t k){
  stringstream s;
  s << t << ":" << k << endl;
  return s.str();
}

string identifier::get_type(source_t s){
  size_t split = s.find(':');
  return s.substr(0, split);
}

source_t identifier::get_string_id(source_t s){
  size_t split = s.find(':');
  string v = s.substr(split + 1, s.length() - split - 1);
  return (source_t)v;
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

