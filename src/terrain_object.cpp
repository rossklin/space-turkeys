#include "terrain_object.h"

#include "utility.h"

using namespace st3;
using namespace std;

pair<int, int> terrain_object::intersects_with(terrain_object obj, float r) const {
  r *= 0.5;
  for (int i = 0; i < border.size(); i++) {
    point p1 = get_vertice(i, r);
    point p2 = get_vertice(i + 1, r);

    for (int j = 0; j < obj.border.size(); j++) {
      point q1 = obj.get_vertice(j, r);
      point q2 = obj.get_vertice(j + 1, r);

      if (utility::line_intersect(p1, p2, q1, q2)) return make_pair(i, j);
    }
  }

  return make_pair(-1, -1);
}

point terrain_object::get_vertice(int idx, float rbuf) const {
  point p = border[utility::int_modulus(idx, border.size())];
  return p + utility::normalize_and_scale(p - center, rbuf);
}

vector<point> terrain_object::get_border(float r) const {
  vector<point> res(border.size());
  for (int i = 0; i < border.size(); i++) res[i] = get_vertice(i, r);
  return res;
}

void terrain_object::set_vertice(int idx, point p) {
  border[utility::int_modulus(idx, border.size())] = p;
}

int terrain_object::triangle(point p, float rad) const {
  float a_sol = utility::point_angle(p - center);
  for (int j = 0; j < border.size(); j++) {
    if (utility::in_triangle(center, get_vertice(j, rad), get_vertice(j + 1, rad), p)) return j;
  }

  return -1;
}

point terrain_object::closest_exit(point p, float r) const {
  float dmin = INFINITY;
  point res;
  float rbuf = 1.01 * r;

  int test_init = triangle(p, r);

  if (test_init == -1) {
    server::log("closest exit: not in terrain!");
    return p;
  }

  for (int i = 0; i < border.size(); i++) {
    point a = get_vertice(i, rbuf);
    point b = get_vertice(i + 1, rbuf);
    point x;
    float d = utility::dpoint2line(p, a, b, &x);

    if (d < dmin) {
      dmin = d;
      res = x;
    }
  }

  int test = triangle(res, r);

  if (test > -1) {
    if (test == test_init) {
      throw logical_error("pushed point still in same triangle!");
    } else {
      return closest_exit(res, r);
    }
  } else {
    return res;
  }
}
