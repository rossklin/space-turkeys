#pragma once

#include <vector>

#include "types.hpp"

namespace st3 {
struct terrain_object {
  class_t type;
  point center;
  std::vector<point> border;

  point get_vertice(int idx, float rbuf = 0) const;
  void set_vertice(int idx, point p);

  std::pair<int, int> intersects_with(terrain_object b, float r = 0) const;
  int triangle(point p, float r) const;
  bool contains(point p, float r) const;
  std::vector<point> get_border(float r) const;
  point closest_exit(point p, float r) const;
};
};  // namespace st3