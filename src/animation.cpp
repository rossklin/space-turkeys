#include "animation.hpp"

#include "client_game.hpp"

using namespace std;
using namespace st3;

animation::animation(const animation_data &d) : animation_data(d) {
  frame = 0;
}

// float animation::time_passed() {
//   return client::frame_time * 10 * (frame - delay) / (float)sub_frames;
// }
