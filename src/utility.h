#ifndef _STK_UTILITY
#define _STK_UTILITY

#include <sstream>
#include <vector>
#include <set>

#include <SFML/Graphics.hpp>

#include "types.h"
#include "graphics.h"

namespace st3{
  /*! Arithmetics for points, vectors, sets and sfml objects. */
  namespace utility{
    /* **************************************** */
    /* POINT MATHS */
    /* **************************************** */

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

    /*! 
      generates a random number normally distributed as (m, s)
      @return the random number
    */
    float random_normal(float m, float s);

    /*! 
      generates a random number uniformly distributed in [0,1]
      @return the random number
    */
    float random_uniform();
    
    /*! 
      generates a vector of uniformly distributed values in [0,1]
      @param n size of vector to generate
      @return the vector
    */
    std::vector<float> random_uniform(int n);

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

    /* **************************************** */
    /* VECTOR MATHS */
    /* **************************************** */

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
    /* SFML STUFF */
    /* **************************************** */

    /*! build and sfml rectangle shape from given bounds
      @param r bounds
      @return an sf::RectangleShape with size and position from r
    */
    sf::RectangleShape build_rect(sf::FloatRect r);

    /*! compute coordinates of the upper left corner of the current view
      @param w window to get the view from
      @return coordinates corresponding to UL corner of current view of w
    */
    point ul_corner(window_t &w);

    /*! compute an sf::Transform mapping from coordinates to pixels
      @param w the current window
      @return the transform
    */
    sf::Transform view_inverse_transform(window_t &w);

    /*! compute the scale factor coordinates per pixel

      such that coord_dims = inverse_scale() * pixel_dims
      
      @param w the current window
      @return point containing x and y scales
    */
    point inverse_scale(window_t &w);

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

  /*! compute set difference
    @param a first set
    @param b second set
    @return set of elements in a not in b
  */
  template<typename T>
    std::set<T> operator - (const std::set<T> &a, const std::set<T> &b);

  /*! remove elements in a set from another set
    @param a set to decrement
    @param b set of elements to remove
    @return a
  */
  template<typename T>
    std::set<T> operator -= (std::set<T> &a, const std::set<T> &b);

  /*! compute union of two sets
    @param a first set
    @param b second set
    @return set of elements in a or b
  */
  template<typename T>
    std::set<T> operator + (const std::set<T> &a, const std::set<T> &b);

  /*! add elements in a set to another set
    @param a set to add elements to
    @param b set of elements to add
    @return resulting set
  */
  template<typename T>
    std::set<T> operator += (std::set<T> &a, const std::set<T> &b);

  /*! compute intersection of two sets
    @param a first set
    @param b second set
    @return set of elements in a and b
  */
  template<typename T>
    std::set<T> operator & (const std::set<T> &a, const std::set<T> &b);
};

#endif
