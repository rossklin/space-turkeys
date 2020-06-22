#include "explosion.h"

#include <chrono>

using namespace std;
using namespace chrono;
using namespace st3;

explosion::explosion(point p, sint c) : position(p), color(c) {
  created = system_clock::now();
}

float explosion::time_passed() {
  return duration<float>(system_clock::now() - created).count();
}
