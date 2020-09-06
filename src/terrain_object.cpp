#include "terrain_object.hpp"

#include "utility.hpp"

using namespace st3;
using namespace std;
using namespace utility;

pair<int, int> terrain_object::intersects_with(terrain_object obj, float r) const {
  r *= 0.5;
  for (int i = 0; i < border.size(); i++) {
    point p1 = get_vertice(i, r);
    point p2 = get_vertice(i + 1, r);

    for (int j = 0; j < obj.border.size(); j++) {
      point q1 = obj.get_vertice(j, r);
      point q2 = obj.get_vertice(j + 1, r);

      if (line_intersect(p1, p2, q1, q2)) return make_pair(i, j);
    }
  }

  return make_pair(-1, -1);
}

point terrain_object::get_vertice(int idx, float rbuf) const {
  point p = border[int_modulus(idx, border.size())];
  return p + normalize_and_scale(p - center, rbuf);
}

vector<point> terrain_object::get_border(float r) const {
  vector<point> res(border.size());
  for (int i = 0; i < border.size(); i++) res[i] = get_vertice(i, r);
  return res;
}

void terrain_object::set_vertice(int idx, point p) {
  border[int_modulus(idx, border.size())] = p;
}

int terrain_object::triangle(point p, float rad) const {
  float a_sol = point_angle(p - center);
  for (int j = 0; j < border.size(); j++) {
    if (in_triangle(center, get_vertice(j, rad), get_vertice(j + 1, rad), p)) return j;
  }

  return -1;
}

bool terrain_object::contains(point p, float r) const {
  auto buf = get_border(r);
  point ul = {INFINITY, INFINITY};
  point br = {-INFINITY, -INFINITY};
  for (auto x : buf) {
    ul = point_min(ul, x);
    br = point_max(br, x);
  }

  if (!point_between(p, ul, br)) return false;

  ul = point_min(ul, p);
  br = point_max(br, p);
  point p_inf = ul - (br - ul);

  int count = 0;
  for (int i = 0; i < buf.size(); i++) {
    int j = int_modulus(i + 1, buf.size());
    count += line_intersect(buf[i], buf[j], p, p_inf);
  }

  return count % 2 == 1;
}

point terrain_object::closest_exit(point p, float r) const {
  if (!contains(p, r)) {
    server::log("closest exit: not in terrain!");
    return p;
  }

  float dmin = INFINITY;
  point res;
  float rbuf = 1.01 * r + 1;
  auto buf = get_border(rbuf);
  auto buf2 = get_border(r);
  for (int i = 0; i < border.size(); i++) {
    int j = int_modulus(i + 1, border.size());
    if (buf[i] == buf2[i] || buf[j] == buf2[j]) {
      throw logical_error("Buffered border is not buffered!");
    }

    point x;
    float d = dpoint2line(p, buf[i], buf[j], &x);

    if (d < dmin) {
      dmin = d;
      res = x;
    }

    if (d == 0 || res == p) {
      throw logical_error("Closest exit found inside border!");
    }
  }

  if (contains(res, r)) {
    if (res == p) {
      throw logical_error("pushed point did not move!");
    } else {
      return closest_exit(res, r);
    }
  } else {
    return res;
  }
}
