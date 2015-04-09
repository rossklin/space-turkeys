#ifndef _STK_UTILITY
#define _STK_UTILITY

#include "types.h"

namespace st3{
  namespace utility{
    // point maths
    float l2norm(point p);
    float point_angle(point p);
    float point_mult(point a, point b);
    point scale_point(point p, float a);
    float sproject(point a, point r);
    float angle_distance(float a, float b);
    float dpoint2line(point p, point a, point b);
  };
  point operator -(const point &a, const point &b);
};

#endif
