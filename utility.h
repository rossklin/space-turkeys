#ifndef _STK_UTILITY
#define _STK_UTILITY

#include <sstream>
#include <vector>

#include <SFML/Graphics.hpp>

#include "types.h"

namespace st3{
  namespace utility{
    // point maths
    float l2norm(point p);
    float l2d2(point p);
    float scalar_mult(point a, point b);
    float point_angle(point p);
    float point_mult(point a, point b);
    point scale_point(point p, float a);
    float sproject(point a, point r);
    float angle_distance(float a, float b);
    float dpoint2line(point p, point a, point b);
    float random_uniform();
    point random_point_polar(point p, float r);
    bool point_above(point p, point q);
    bool point_below(point p, point q);
    bool point_between(point a, point p, point q);
    point normv(float angle);

    // vector maths
    void normalize_vector(std::vector<float> &x);

    // sfml stuff
    sf::RectangleShape build_rect(sf::FloatRect r);
    
    float sigmoid(float x);
    float modulus(float x, float p);
  };
  point operator -(const point &a, const point &b);
  point operator +(const point &a, const point &b);
  std::ostream & operator << (std::ostream &ss, std::vector<float> const &x);
};

#endif
