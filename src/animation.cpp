#include <chrono>

#include "animation.h"

using namespace std;
using namespace chrono;
using namespace st3;

animation::animation(const animation_data &d) : animation_data(d) {
  created = system_clock::now();
}

float animation::time_passed() {
  return duration<float>(system_clock::now() - created).count();
}
