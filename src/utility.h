#ifndef _STK_UTILITY
#define _STK_UTILITY

#include <sstream>
#include <vector>
#include <set>
#include <cassert>
#include <rapidjson/document.h>

#include "types.h"
#include "game_object.h"

namespace st3{
  /*! Arithmetics for points, vectors, sets and sfml objects. */
  namespace utility{

    template<typename T, typename F = game_object>
    typename T::ptr guaranteed_cast(typename F::ptr p){
      if (p == 0) throw std::runtime_error("attempt_cast: null pointer!");
  
      typename T::ptr res = dynamic_cast<typename T::ptr>(p);

      if (res){
	return res;
      }else{
	throw std::runtime_error("Failed to downcast");
      }
    }

    template<typename K, typename V>
    std::list<K> get_map_keys(const hm_t<K,V> &m) {
      std::list<K> res;
      for (auto &x : m) res.push_back(x.first);
      return res;
    }

    template<typename M, typename C>
    void assign_keys(M &m, C &data){
      std::transform(m.begin(), m.end(),
		inserter(data, data.end()),
		[](auto pair){ return pair.first; });
    }
    
    template<typename T>
    std::function<T(T)> identity_function() {
      return [](T x){return x;};
    }

    std::string format_float(float x);

    void init();

    rapidjson::Document *get_json(std::string filename);
    
    /* **************************************** */
    /* POINT MATHS */
    /* **************************************** */

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

    /*!
      computes the shortest distance between a point and a line
      @param p the point
      @param a,b end points of the line
      @return shortest distance between p and the line from a to b
    */
    float dpoint2line(point p, point a, point b);

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

    template<typename T>
    T uniform_sample(std::list<T> &x){
      int s = x.size();

      for (auto &y : x){
	if (random_uniform() <= 1 / (float)(s--)) return y;
      }

      return x.back();
    }

    template<typename T>
    int vector_min(std::vector<T> &x, std::function<float(T)> h) {
      if (x.empty()) {
	throw std::runtime_error("Can not call vector_min with empty vector!");
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
	throw std::runtime_error("vector_min: no finite values found!");
      }

      return idx;
    }
    
    template<typename T, typename C>
    T value_min(const C &x, std::function<float(T)> h) {
      if (x.empty()) {
	throw std::runtime_error("Can not call value_min with empty vector!");
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

      return result;
    }

    int angle2index(int na, float a);
    float index2angle(int na, int idx);
    std::vector<float> circular_kernel(const std::vector<float> &x, float s = 1);

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

    float signum(float x, float eps = 0);

    float mass2area(float m);

    float safe_inv(float x);

    /* **************************************** */
    /* VECTOR MATHS */
    /* **************************************** */

    template<typename T>
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
    
    /*! apply a smooth limit to a value
      @param x value to limit
      @param s limit to apply, s.t. -s < sigmoid(x,s) < s
      @return limited value
    */
    float sigmoid(float x, float s = 1);
    
    /*! compute modulus 
      @param x value
      @param p modulus
      @return x mod p
    */
    float modulus(float x, float p);

    int int_modulus(int x, int p);

    std::vector<int> sequence(int a, int b);

    /*! generate colors distinct from each other and from black and grey
      @param n number of colors 
      @return n distinct colors in 32 bit argb format
    */
    std::vector<sint> different_colors(int n);
  };

  /*! compute difference between two points
    @param a first point
    @param b second point
    @return point(a.x - b.x, a.y - b.y)
  */
  point operator - (const point &a, const point &b);
  
  /*! compute sum of two points
    @param a first point
    @param b second point
    @return point(a.x + b.x, a.y + b.y)
  */
  point operator + (const point &a, const point &b);

  /*! print vector to output stream
    @param ss output stream
    @param x vector
    @return reference to stream
  */
  std::ostream & operator << (std::ostream &ss, std::vector<float> const &x);

  /*! print point to output stream
    @param ss output stream
    @param x point
    @return reference to stream
  */
  std::ostream & operator << (std::ostream &ss, point const &x);

  /* **************************************** */
  /* SET OPERATIONS */
  /* **************************************** */
  
  // elements in a not in b
  template<typename T>
  std::set<T> operator - (const std::set<T> &a, const std::set<T> &b){
    std::set<T> res = a;
    for (auto &x : b) res.erase(x);
    return res;
  }

  // remove elements in b from a
  template<typename T>
  std::set<T> operator -= (std::set<T> &a, const std::set<T> &b){
    for (auto &x : b) a.erase(x);
    return a;
  }

  // elements in a or b
  template<typename T>
  std::set<T> operator + (const std::set<T> &a, const std::set<T> &b){
    std::set<T> res = a;
    for (auto &x : b) res.insert(x);
    return res;
  }

  // add elements in b to a
  template<typename T>
  std::set<T> operator += (std::set<T> &a, const std::set<T> &b){
    for (auto &x : b) a.insert(x);
    return a;
  }

  // elements in a and b
  template<typename T>
  std::set<T> operator & (const std::set<T> &a, const std::set<T> &b){
    std::set<T> res;
    for (auto &x : b) {
      if (a.count(x)) res.insert(x);
    }
    return res;
  }

};

#endif
