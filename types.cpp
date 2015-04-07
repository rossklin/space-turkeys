#include "types.h"

using namespace std;
using namespace st3;

size_t st3::source_t::hash::operator()(const source_t &a) const{
  return a.id * (1 - 2 * a.type);
}

bool st3::source_t::pred::operator()(const source_t &a,const  source_t &b) const{
  return a.type == b.type && a.id == b.id;
}
