#include <iomanip>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <sstream>
#include <rapidjson/document.h>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/normal_distribution.hpp>

#include "utility.h"
#include "ship.h"
#include "solar.h"
#include "fleet.h"
#include "waypoint.h"
#include "game_object.h"
#include "player.h"

using namespace std;
using namespace st3;
using namespace st3::utility;

boost::random::mt19937 rng;

void utility::init(){
  rng.seed(time(NULL));
}

string get_file(ifstream& in) {
    stringstream sstr;
    sstr << in.rdbuf();
    return sstr.str();
}

rapidjson::Document *utility::get_json(string key) {
  string filename = key + "_data.json";
  ifstream file(filename);
  string json_data = get_file(file);
  rapidjson::Document *doc = new rapidjson::Document();
  doc -> Parse(json_data.c_str());
  if (doc -> HasParseError()) {
    cout << "Error parsing json in " << filename << endl;
    cout << "Code: " << doc -> GetParseError() << endl;
    cout << "Offset: " << doc -> GetErrorOffset() << endl;
    cout << "JSON data: " << endl << json_data << endl;
    exit(-1);
  }
  
  if (!(doc -> HasMember(key.c_str()) && (*doc)[key.c_str()].IsObject())) throw runtime_error("Invalid data in " + filename + "!");
  return doc;
}

template<typename T, typename F>
typename T::ptr utility::guaranteed_cast(typename F::ptr p){
  if (p == 0) throw runtime_error("attempt_cast: null pointer!");
  
  typename T::ptr res = dynamic_cast<typename T::ptr>(p);

  if (res){
    return res;
  }else{
    throw runtime_error("Failed to downcast");
  }
}

template<typename K, typename V>
list<K> utility::get_map_keys(const hm_t<K,V> &m) {
  list<K> res;
  for (auto &x : m) res.push_back(x.first);
  return res;
}

template list<string> utility::get_map_keys<string, ship_stats>(const hm_t<string, ship_stats>&);


template<typename M, typename C>
void utility::assign_keys(M &m, C &data){
  transform(m.begin(), m.end(),
	    inserter(data, data.end()),
	    [](auto pair){ return pair.first; });
}

template void utility::assign_keys<hm_t<int, player>, vector<int> > (hm_t<int, player>&, vector<int>&);

// ****************************************
// POINT ARITHMETICS
// ****************************************

float utility::signum(float x, float eps){
  return (x > eps) - (x < -eps);
}

// scalar product of points a and b
float utility::scalar_mult(point a, point b){
  return a.x * b.x + a.y * b.y;
}

// squared l2 norm of point p
float utility::l2d2(point p){
  return scalar_mult(p, p);
}

// l2 norm of point p
float utility::l2norm(point p){
  return sqrt(l2d2(p));
}

// angle between point p and x-axis
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

// scale point p by factor a
point utility::scale_point(point p, float a){
  return point(a * p.x, a * p.y);
}

// scalar projection coefficient of point a on r
float utility::sproject(point a, point r){
  return scalar_mult(a,r) / scalar_mult(r,r);
}


float utility::angle_difference(float a, float b){
  return modulus(a - b + M_PI, 2 * M_PI) - M_PI;
}

// shortest distance between angles a and b
float utility::angle_distance(float a, float b){
  return fabs(angle_difference(a,b));
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

// point difference operator
point st3::operator -(const point &a, const point &b){
  return point(a.x - b.x, a.y - b.y);
}

// point addition operator
point st3::operator +(const point &a, const point &b){
  return point(a.x + b.x, a.y + b.y);
}


template<typename T>
T utility::uniform_sample(std::list<T> &x){
  int s = x.size();

  for (auto &y : x){
    if (random_uniform() <= 1 / (float)(s--)) return y;
  }

  return x.back();
}

// normal ~N(m,s)
float utility::random_normal(float m, float s){
  boost::random::normal_distribution<float> dist(m,s);
  return dist(rng);
}

// random float ~U(a,b)
float utility::random_uniform(float a, float b){
  boost::random::uniform_real_distribution<float> dist(a,b);
  return dist(rng);
}

// random uniform int in [0, limit)
unsigned int utility::random_int(int limit){
  if (limit < 2) return 0;
  boost::random::uniform_int_distribution<> dist(0, limit - 1);
  return dist(rng);
}

// vector of n random floats ~U(0,1) 
vector<float> utility::random_uniform_vector(int n, float a, float b){
  boost::random::uniform_real_distribution<float> dist(a,b);
  vector<float> res(n);
  for (auto &x : res) x = dist(rng);
  return res;
}

// random point with gaussian distribution around p, sigma = r
point utility::random_point_polar(point p, float r){
  boost::random::normal_distribution<float> n(0, r);
  return point(p.x + n(rng), p.y + n(rng));
}

// p geq q both dims
bool utility::point_above(point p, point q){
  return p.x >= q.x && p.y >= q.y;
}

// p lt q both dims
bool utility::point_below(point p, point q){
  return p.x < q.x && p.y < q.y;
}

// a above p and below q
bool utility::point_between(point a, point p, point q){
  return point_above(a, p) && point_below(a, q);
}

// normalized point with angle a to x-axis
point utility::normv(float a){
  return point(cos(a), sin(a));
}

// vector maths
// normalize the vector
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

// l2norm of the vector
float utility::vl2norm(vector<float> const &x){
  float ss = 0;
  for (auto &z : x) ss += z * z;
  return sqrt(ss);
}

// vector difference
vector<float> utility::vdiff(vector<float> const &a, vector<float> const &b){
  if (a.size() != b.size()){
    throw runtime_error("vdiff: dimension mismatch");
  }

  vector<float> res = a;
  for (int i = 0; i < res.size(); i++){
    res[i] -= b[i];
  }

  return res;
}

// vector addition
vector<float> utility::vadd(vector<float> const &a, vector<float> const &b){
  if (a.size() != b.size()){
    throw runtime_error("vdiff: dimension mismatch");
  }

  vector<float> res = a;
  for (int i = 0; i < res.size(); i++){
    res[i] += b[i];
  }

  return res;
}

// rescale vector a by s
vector<float> utility::vscale(vector<float> a, float s){
  for (auto &x : a) x *= s;
  return a;
}

// sigmoid function: approaches -s below -s and s above s, like
// identity function between
float utility::sigmoid(float x, float s){
  return s * atan(x / s) / (M_PI / 2);
}

// generate a vector of n different colors, which are also different
// from black (background) and grey (neutral solar) colors
vector<sint> utility::different_colors(int n){
  int rep = 40;
  int ncheck = n+2;
  vector<vector<float> > buf(ncheck);

  for (auto &x : buf) {
    x = utility::random_uniform_vector(3);
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
	buf[j] = utility::vadd(buf[j], utility::vscale(delta, 0.05));
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
    res[i] = (red << 24) | (green << 16) | (blue << 8) | 0xff;
  }

  return res;
}

// return x mod p, presuming p > 0
float utility::modulus(float x, float p){
  int num = floor(x / p);
  return x - num * p;
}

// format a float as low prec. string
string utility::format_float(float x){
  stringstream stream;
  stream << fixed << setprecision(2) << x;
  return stream.str();
}

// output vector x to stream ss
ostream & st3::operator << (ostream &ss, vector<float> const &x){
  for (auto y : x) ss << y << ", ";
  return ss;
}

// output point x to stream ss
ostream & st3::operator << (ostream &ss, point const &x){
  ss << "(" << x.x << ", " << x.y << ")" << endl;
  return ss;
}

// set arithmetic operations

// elements in a not in b
template<typename T>
set<T> st3::operator - (const set<T> &a, const set<T> &b){
  set<T> res = a;
  for (auto &x : b) res.erase(x);
  return res;
}

// remove elements in b from a
template<typename T>
set<T> st3::operator -= (set<T> &a, const set<T> &b){
  for (auto &x : b) a.erase(x);
  return a;
}

// elements in a or b
template<typename T>
set<T> st3::operator + (const set<T> &a, const set<T> &b){
  set<T> res = a;
  for (auto &x : b) res.insert(x);
  return res;
}

// add elements in b to a
template<typename T>
set<T> st3::operator += (set<T> &a, const set<T> &b){
  for (auto &x : b) a.insert(x);
  return a;
}

// elements in a and b
template<typename T>
set<T> st3::operator & (const set<T> &a, const set<T> &b){
  set<T> res;
  for (auto &x : b) {
    if (a.count(x)) res.insert(x);
  }
  return res;
}

// random sample instantiation
template combid st3::utility::uniform_sample(list<combid>&);

// set operations instantiation
template set<combid> st3::operator - (const set<combid> &a, const set<combid> &b);
template set<combid> st3::operator -= (set<combid> &a, const set<combid> &b);
template set<combid> st3::operator + (const set<combid> &a, const set<combid> &b);
template set<combid> st3::operator += (set<combid> &a, const set<combid> &b);
template set<combid> st3::operator & (const set<combid> &a, const set<combid> &b);

// shared pointer cast instantiation
template ship::ptr utility::guaranteed_cast<ship>(game_object::ptr);
template fleet::ptr utility::guaranteed_cast<fleet>(game_object::ptr);
template solar::ptr utility::guaranteed_cast<solar>(game_object::ptr);
template waypoint::ptr utility::guaranteed_cast<waypoint>(game_object::ptr);
template commandable_object::ptr utility::guaranteed_cast<commandable_object>(game_object::ptr);
template physical_object::ptr utility::guaranteed_cast<physical_object>(game_object::ptr);
