#pragma once

#include <rapidjson/document.h>

#include <SFML/Network.hpp>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <set>
#include <sstream>
#include <vector>

#include "types.hpp"

namespace st3 {
namespace utility {

template <typename T, typename F>
T range_init(const F &x) {
  T res(x.begin(), x.end());
  return res;
}

template <typename C2, typename C1>
C2 range_map(std::function<typename C2::value_type(typename C1::value_type)> f, C1 x) {
  C2 res(x.size());
  transform(x.begin(), x.end(), res.begin(), f);
  return res;
}

template <typename K, typename V, typename V2>
hm_t<K, V2> hm_map(std::function<V2(K, V)> f, hm_t<K, V> x) {
  hm_t<K, V2> res;
  for (auto &y : x) res[y.first] = f(y.first, y.second);
  return res;
}

template <typename T>
bool any(T c) {
  for (auto x : c) {
    if (x) return true;
  }
  return false;
}

template <typename T>
bool all(T c) {
  for (auto x : c) {
    if (!x) return false;
  }
  return true;
}

template <typename K, typename V>
std::vector<K> hm_keys(hm_t<K, V> x) {
  std::vector<K> res;
  for (auto y : x) res.push_back(y.first);
  return res;
}

template <typename K, typename V>
std::vector<V> hm_values(hm_t<K, V> x) {
  std::vector<V> res;
  for (auto y : x) res.push_back(y.second);
  return res;
}

/*! Arithmetics for points, vectors, sets and sfml objects. */
const std::string root_path = "/usr/share/spaceturkeys-3/";

template <typename T>
T filter(T con, std::function<bool(typename T::value_type)> f) {
  T res;
  for (auto x : con) {
    if (f(x)) res.push_back(x);
  }
  return res;
}

template <typename T, typename F = game_object>
typename T::ptr guaranteed_cast(typename F::ptr p) {
  if (p == 0) throw logical_error("attempt_cast: null pointer!");

  typename T::ptr res = std::dynamic_pointer_cast<T>(p);

  if (res) {
    return res;
  } else {
    throw classified_error("Failed to downcast", "error");
  }
}

template <typename M, typename C>
void assign_keys(M &m, C &data) {
  std::transform(m.begin(), m.end(),
                 inserter(data, data.end()),
                 [](auto pair) { return pair.first; });
}

template <typename C>
bool find_in(typename C::value_type x, C c) {
  return std::find(c.begin(), c.end(), x) != c.end();
}

template <typename T>
std::function<T(T)> identity_function() {
  return [](T x) { return x; };
}

template <
    typename C2,
    typename C1,
    typename A = typename C2::value_type,
    typename B = typename C1::value_type>
C2 fmap(C1 &x, std::function<A(B)> f) {
  C2 res;
  for (auto &y : x) res.push_back(f(y));
  return res;
}

template <typename C>
typename C::value_type vsum(C &x) {
  typename C::value_type sum = 0;
  for (auto y : x) sum += y;
  return sum;
}

std::string format_float(float x);

rapidjson::Document *get_json(std::string filename);

/* **************************************** */
/* POINT MATHS */
/* **************************************** */

std::vector<point> cluster_points(std::vector<point> x, int n = 10, int rep = 10, float h = 50);

std::string point2string(point p);

/*! 
      computes the l2 norm of a point
      @param[in] p point to compute l2 norm for
      @return ||p||_2
     */
float l2norm(point p);

/*! 
      computes the squared l2 norm of a point
      @param[in] p point to compute squared l2 norm for
      @return ||p||^2
     */
float l2d2(point p);

/*!
      computes the scalar product of two points
      @param a,b points to multiply
      @return a*b
    */
float scalar_mult(point a, point b);

/*! 
      computes the angle between a vector and the x-axis
      @param p point to compute angle for
      @return angle between p and x-axis
    */
float point_angle(point p);

/*! 
      computes a point scaled
      @param p point to scale
      @param a scale factor
      @return p scaled by factor a
    */
point scale_point(point p, float a);

/*!
      computes the scalar projection coefficient of a point on another
      @param a point to project
      @param r vector to project a on
      @return (a * r) / (r * r)
    */
float sproject(point a, point r);

float angle_difference(float a, float b);

/*!
      computes the shortest distance between two angles on a circle
      @param a first angle
      @param b second angle
      @return shortest (forward or backward) distance between a and b
    */
float angle_distance(float a, float b);
bool in_triangle(point a, point b, point c, point x);
float triangle_relative_distance(point c, point b1, point b2, point x);
bool line_intersect(point a, point b, point p1, point p2, point *r = NULL);

/*!
      computes the shortest distance between a point and a line
      @param p the point
      @param a,b end points of the line
      @return shortest distance between p and the line from a to b
    */
float dpoint2line(point p, point a, point b, point *r = NULL);

unsigned int random_int(int limit);

/*! 
      generates a random number normally distributed as (m, s)
      @return the random number
    */
float random_normal(float m = 0, float s = 1);

/*! 
      generates a random number uniformly distributed in [a, b]
      @param a lower bound
      @param b upper bound
      @return the random number
    */
float random_uniform(float a = 0, float b = 1);

/*! 
      generates a vector of uniformly distributed values in [0,1]
      @param n size of vector to generate
      @return the vector
    */
std::vector<float> random_uniform_vector(int n, float a = 0, float b = 1);

template <typename C>
typename C::key_type weighted_sample(C x) {
  if (x.empty()) {
    throw logical_error("weighted_sample: empty map!");
  }

  typename C::key_type def;
  float sum = 0;
  for (auto &y : x) sum += y.second;
  if (sum == 0) return def;

  float targ = random_uniform(0, sum);

  sum = 0;
  for (auto &y : x) {
    sum += y.second;
    def = y.first;
    if (sum >= targ) break;
  }

  return def;
};

template <typename C>
typename C::key_type sq_weighted_sample(C x) {
  for (auto &y : x) y.second = pow(y.second, 2);
  return weighted_sample(x);
};

template <typename C>
typename C::value_type uniform_sample(C &x) {
  int s = x.size();
  typename C::value_type test;

  for (auto &y : x) {
    test = y;
    if (random_uniform() <= 1 / (float)(s--)) return y;
  }

  return test;
}

template <typename T>
int vector_min(std::vector<T> &x, std::function<float(T)> h) {
  if (x.empty()) {
    throw logical_error("Can not call vector_min with empty vector!");
  }

  float best = INFINITY;
  int idx = -1;
  for (int i = 0; i < x.size(); i++) {
    float value = h(x[i]);
    if (value < best) {
      best = value;
      idx = i;
    }
  }

  if (idx == -1) {
    throw logical_error("vector_min: no finite values found!");
  }

  return idx;
}

template <typename C, typename T = typename C::value_type>
T value_min(const C &x, std::function<float(T)> h) {
  if (x.empty()) {
    throw logical_error("Can not call value_min with empty vector!");
  }

  float best = INFINITY;
  T result;
  for (auto &y : x) {
    float value = h(y);
    if (value < best) {
      best = value;
      result = y;
    }
  }

  if (!std::isfinite(best)) {
    throw logical_error("value_min: no finite value found!");
  }

  return result;
}

int angle2index(int na, float a);
float index2angle(int na, int idx);
std::vector<float> circular_kernel(const std::vector<float> &x, float s = 1);

float gaussian_kernel(float x, float s = 1);

float interval_function(float a, float b, float x);

float angular_hat(float x);

/*! 
      generates a random point with gaussian distribution 
      @param p center point
      @param r standard deviation
      @return the point
    */
point random_point_polar(point p, float r);

/*!
      checks if point p is greater than or equal to point q in both dimensions
      @param p first point
      @param q second point
      @return p.x >= q.x && p.y >= q.y
    */
bool point_above(point p, point q);

/*!
      checks if point p is less than point q in both dimensions
      @param p first point
      @param q second point
      @return p.x < q.x && p.y < q.y
    */
bool point_below(point p, point q);

/*!
      checks if a point is between two points
      @param a point to check
      @param p lower bound point
      @param q upper bound point
      @return point_above(a,p) && point_below(a,q)
     */
bool point_between(point a, point p, point q);

/*! 
      generates a point of length 1 with given angle
      @param angle the angle
      @return the point
    */
point normv(float angle);

point normalize_and_scale(point p, float a);

float signum(float x, float eps = 0);

float mass2area(float m);

float safe_inv(float x);

/* **************************************** */
/* VECTOR MATHS */
/* **************************************** */

template <typename T>
T cubic_interpolation(std::vector<T> data, float t) {
  assert(data.size() == 4);
  float sixth = 1 / (float)6;
  T a1 = sixth * (data[3] - (float)3 * data[2] + (float)3 * data[1] - data[0]);
  T a2 = (float)0.5 * (data[0] + data[2] - (float)2 * data[1]);
  T a3 = sixth * (-data[3] + (float)6 * data[2] - (float)3 * data[1] - (float)2 * data[0]);
  T a4 = data[1];
  return (float)pow(t, 3) * a1 + (float)pow(t, 2) * a2 + t * a3 + a4;
}

template <typename T>
std::vector<T> elementwise_product(const std::vector<T> &a, const std::vector<T> &b) {
  assert(a.size() == b.size());
  std::vector<T> x = a;
  for (int i = 0; i < x.size(); i++) x[i] *= b[i];
  return x;
};

/*! normalize a vector
      @param x vector to normalize
    */
void normalize_vector(std::vector<float> &x);

/*! compute difference between two vectors
      @param a first vector
      @param b second vector
      @return vector of elementwise differences between a and b
    */
std::vector<float> vdiff(std::vector<float> const &a, std::vector<float> const &b);

/*! compute sum of two vectors
      @param a first vector
      @param b second vector
      @return vector of elementwise sum of a and b
     */
std::vector<float> vadd(std::vector<float> const &a, std::vector<float> const &b);

/*! compute scaled vector
      @param a vector to scale
      @param s scale factor
      @return vector of elements in a scaled by s
    */
std::vector<float> vscale(std::vector<float> a, float s);

/*! compute l2norm of a vector
      @param x vector to compute norm for
      @return square root of sum of squares of x
    */
float vl2norm(std::vector<float> const &x);

/* **************************************** */
/* MATHS */
/* **************************************** */

point p2solve(float a, float b, float c);

/*! apply a smooth limit to a value
      @param x value to limit
      @param s limit to apply, s.t. -s < sigmoid(x,s) < s
      @return limited value
    */
float sigmoid(float x, float s = 1);

float linsig(float x);

/*! compute modulus 
      @param x value
      @param p modulus
      @return x mod p
    */
float modulus(float x, float p);

int int_modulus(int x, int p);

std::vector<int> sequence(int a, int b);

std::vector<int> zig_seq(int a);

/*! generate colors distinct from each other and from black and grey
      @param n number of colors 
      @return n distinct colors in 32 bit argb format
    */
std::vector<sint> different_colors(int n);
};  // namespace utility

/*! compute difference between two points
    @param a first point
    @param b second point
    @return point(a.x - b.x, a.y - b.y)
  */
point operator-(const point &a, const point &b);

/*! compute sum of two points
    @param a first point
    @param b second point
    @return point(a.x + b.x, a.y + b.y)
  */
point operator+(const point &a, const point &b);

point operator*(const float a, const point b);

/*! print vector to output stream
    @param ss output stream
    @param x vector
    @return reference to stream
  */
template <typename T>
std::ostream &operator<<(std::ostream &ss, std::vector<T> const &x) {
  for (auto y : x) ss << y << ", ";
  return ss;
}

template <typename T1, typename T2>
std::ostream &operator<<(std::ostream &ss, std::pair<T1, T2> const &x) {
  return ss << "{" << x.first << ", " << x.second << "}";
}

/*! print point to output stream
    @param ss output stream
    @param x point
    @return reference to stream
  */
std::ostream &operator<<(std::ostream &ss, point const &x);

/* **************************************** */
/* SET OPERATIONS */
/* **************************************** */

// elements in a not in b
template <typename T>
std::set<T> operator-(const std::set<T> &a, const std::set<T> &b) {
  std::set<T> res = a;
  for (auto &x : b) res.erase(x);
  return res;
}

// remove elements in b from a
template <typename T>
std::set<T> operator-=(std::set<T> &a, const std::set<T> &b) {
  for (auto &x : b) a.erase(x);
  return a;
}

// elements in a or b
template <typename T>
std::set<T> operator+(const std::set<T> &a, const std::set<T> &b) {
  std::set<T> res = a;
  for (auto &x : b) res.insert(x);
  return res;
}

// add elements in b to a
template <typename T>
std::set<T> operator+=(std::set<T> &a, const std::set<T> &b) {
  for (auto &x : b) a.insert(x);
  return a;
}

// elements in a and b
template <typename T>
std::set<T> operator&(const std::set<T> &a, const std::set<T> &b) {
  std::set<T> res;
  for (auto &x : b) {
    if (a.count(x)) res.insert(x);
  }
  return res;
}

// SFML wrappers
packet_ptr protopack(protocol_t p);

};  // namespace st3
