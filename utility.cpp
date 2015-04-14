#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_01.hpp>

#include "utility.h"

using namespace st3;
using namespace st3::utility;

boost::random::mt19937 rng;
boost::random::uniform_01<float> unidist;

// ****************************************
// POINT ARITHMETICS
// ****************************************

float utility::l2norm(point p){
  return sqrt(pow(p.x, 2) + pow(p.y, 2));
}

float utility::point_angle(point p){
  if (p.x > 0){
    return atan(p.y / p.x);
  }else if (p.x < 0){
    return M_PI + atan(p.y / p.x);
  }else if (p.y > 0){
    return M_PI / 2;
  }else {
    return -M_PI / 2;
  }
}

float utility::point_mult(point a, point b){
  return a.x * b.x + a.y * b.y;
}

point utility::scale_point(point p, float a){
  return point(a * p.x, a * p.y);
}

float utility::sproject(point a, point r){
  return point_mult(a,r) / point_mult(r,r);
}

// shortest distance between angles a and b
float utility::angle_distance(float a, float b){
  float r;

  a = fmod(a, 2 * M_PI);
  b = fmod(a, 2 * M_PI);

  r = fabs(b - a);

  if (r > M_PI){
    r = 2 * M_PI - r;
  }

  return r;
}

// shortest distance between p and line from a to b
float utility::dpoint2line(point p, point a, point b){
  if (angle_distance(point_angle(p - a), point_angle(b - a)) > M_PI/2){
    return l2norm(p - a);
  }else if (angle_distance(point_angle(p - b), point_angle(a - b)) > M_PI/2){
    return l2norm(p - b);
  }else{
    return l2norm(p - scale_point(b - a, sproject(p-a,b-a)) - a);
  }
}

point st3::operator -(const point &a, const point &b){
  return point(a.x - b.x, a.y - b.y);
}

point st3::operator +(const point &a, const point &b){
  return point(a.x + b.x, a.y + b.y);
}

float utility::random_uniform(){
  return unidist(rng);
}

point utility::random_point_polar(point p, float r){
  float angle = 2 * M_PI * random_uniform();
  float radius = random_uniform();
  return p + point(radius * cos(angle), radius * sin(angle));
}
