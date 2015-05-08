#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/normal_distribution.hpp>

#include "utility.h"

using namespace std;
using namespace st3;
using namespace st3::utility;

boost::random::mt19937 rng;
boost::random::uniform_01<float> unidist;

// ****************************************
// POINT ARITHMETICS
// ****************************************

float utility::scalar_mult(point a, point b){
  return a.x * b.x + a.y * b.y;
}

float utility::l2d2(point p){
  return scalar_mult(p, p);
}

float utility::l2norm(point p){
  return sqrt(l2d2(p));
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

  r = modulus(b - a, 2 * M_PI);

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
    float s = sproject(p-a,b-a) / sproject(b-a, b-a);
    return l2norm(p - scale_point(b - a, s) - a);
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

vector<float> utility::random_uniform(int n){
  vector<float> res(n);
  for (auto &x : res) x = unidist(rng);
  return res;
}

point utility::random_point_polar(point p, float r){
  boost::random::normal_distribution<float> n(0, r);
  return point(p.x + n(rng), p.y + n(rng));
}

bool utility::point_above(point p, point q){
  return p.x >= q.x && p.y >= q.y;
}

bool utility::point_below(point p, point q){
  return p.x < q.x && p.y < q.y;
}

bool utility::point_between(point a, point p, point q){
  return point_above(a, p) && point_below(a, q);
}

point utility::normv(float a){
  return point(cos(a), sin(a));
}

// sfml stuff
sf::RectangleShape utility::build_rect(sf::FloatRect bounds){
  sf::RectangleShape r;
  r.setSize(sf::Vector2f(bounds.width, bounds.height));
  r.setPosition(sf::Vector2f(bounds.left, bounds.top));
  return r;
}

// point coordinates of view ul corner
point utility::ul_corner(sf::RenderWindow &w){
  sf::View v = w.getView();
  return v.getCenter() - utility::scale_point(v.getSize(), 0.5);
}

// transform from pixels to points
sf::Transform utility::view_inverse_transform(sf::RenderWindow &w){
  sf::Transform t;
  sf::View v = w.getView();

  t.translate(ul_corner(w));
  t.scale(v.getSize().x / w.getSize().x, v.getSize().y / w.getSize().y);
  return t;
}

// transform from pixels to points
point utility::inverse_scale(sf::RenderWindow &w){
  sf::View v = w.getView();
  return point(v.getSize().x / w.getSize().x, v.getSize().y / w.getSize().y);
}

// vector maths

void utility::normalize_vector(vector<float> &x){
  if (x.empty()) return;

  float sum = 0;

  for (auto y : x) sum += y;

  if (sum){
    sum = abs(sum);
    for (auto &y : x) y /= sum;
  }else{
    for (auto &y : x) y = 0;
  }
}

float utility::vl2norm(vector<float> const &x){
  float ss = 0;
  for (auto &z : x) ss += z * z;
  return sqrt(ss);
}

vector<float> utility::vdiff(vector<float> const &a, vector<float> const &b){
  if (a.size() != b.size()){
    cout << "vdiff: dimension mismatch" << endl;
    exit(-1);
  }

  vector<float> res = a;
  for (int i = 0; i < res.size(); i++){
    res[i] -= b[i];
  }

  return res;
}

vector<float> utility::vadd(vector<float> const &a, vector<float> const &b){
  if (a.size() != b.size()){
    cout << "vdiff: dimension mismatch" << endl;
    exit(-1);
  }

  vector<float> res = a;
  for (int i = 0; i < res.size(); i++){
    res[i] += b[i];
  }

  return res;
}

vector<float> utility::vscale(vector<float> a, float s){
  for (auto &x : a) x *= s;
  return a;
}

float utility::sigmoid(float x, float s){
  return s * atan(x / s) / (M_PI / 2);
}

vector<sint> utility::different_colors(int n){
  int rep = 100;
  int ncheck = n+2;
  vector<vector<float> > buf(ncheck);

  for (auto &x : buf) {
    x = utility::random_uniform(3);
  }

  buf[n] = {0,0,0}; // space color
  buf[n+1] = {150 / (float)256, 150 / (float)256, 150 / (float)256}; // neutral color

  for (int i = 0; i < rep; i++){
    for (int j = 0; j < n; j++){
      float dmin = 10;
      int t = -1;
      for (int k = 0; k < ncheck; k++){
	if (j != k){
	  float d = utility::vl2norm(utility::vdiff(buf[j], buf[k]));
	  if (d < dmin){
	    dmin = d;
	    t = k;
	  }
	}
      }

      if (t > -1){
	// move point j away from point t
	vector<float> delta = utility::vdiff(buf[j], buf[t]);
	buf[j] = utility::vadd(buf[j], utility::vscale(delta, 0.1));
	for (int k = 0; k < 3; k++) buf[j][k] = fmin(fmax(buf[j][k], 0), 1);
      }
    }
  }

  // compile result
  vector<sint> res(n);
  for (int i = 0; i < n; i++){
    sint red = buf[i][0] * 255;
    sint green = buf[i][1] * 255;
    sint blue = buf[i][2] * 255;
    res[i] = (red << 16) | (green << 8) | blue | 0xff000000;
  }

  return res;
}

// p > 0
float utility::modulus(float x, float p){
  int num = floor(x / p);
  return x - num * p;
}

ostream & st3::operator << (ostream &ss, vector<float> const &x){
  for (auto y : x) ss << y << ", ";
  return ss;
}

ostream & st3::operator << (ostream &ss, point const &x){
  ss << "(" << x.x << ", " << x.y << ")" << endl;
  return ss;
}

// set operations
template<typename T>
set<T> st3::operator - (const set<T> &a, const set<T> &b){
  set<T> res = a;
  for (auto &x : b) res.erase(x);
  return res;
}

template<typename T>
set<T> st3::operator -= (set<T> &a, const set<T> &b){
  for (auto &x : b) a.erase(x);
  return a;
}

template<typename T>
set<T> st3::operator + (const set<T> &a, const set<T> &b){
  set<T> res = a;
  for (auto &x : b) res.insert(x);
  return res;
}

template<typename T>
set<T> st3::operator += (set<T> &a, const set<T> &b){
  for (auto &x : b) a.insert(x);
  return a;
}

template<typename T>
set<T> st3::operator & (const set<T> &a, const set<T> &b){
  set<T> res;
  for (auto &x : b) {
    if (a.count(x)) res.insert(x);
  }

  return res;
}


template<typename T>
set<T> st3::operator | (const set<T> &a, const set<T> &b){
  set<T> res;
  for (auto &x : b) res.insert(x);
  for (auto &x : a) res.insert(x);
  return res;
}

// set operations instantiation
template set<idtype> st3::operator - (const set<idtype> &a, const set<idtype> &b);
template set<idtype> st3::operator -= (set<idtype> &a, const set<idtype> &b);
template set<idtype> st3::operator + (const set<idtype> &a, const set<idtype> &b);
template set<idtype> st3::operator += (set<idtype> &a, const set<idtype> &b);
template set<idtype> st3::operator & (const set<idtype> &a, const set<idtype> &b);
template set<idtype> st3::operator | (const set<idtype> &a, const set<idtype> &b);
