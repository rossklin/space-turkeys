#include "utility.h"

#include <rapidjson/document.h>

#include <algorithm>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <mutex>
#include <sstream>

#include "fleet.h"
#include "game_object.h"
#include "player.h"
#include "ship.h"
#include "solar.h"
#include "waypoint.h"

using namespace std;
using namespace st3;
using namespace st3::utility;

template <typename V, typename D>
V get_random(D &dist) {
  static boost::random::mt19937 rng;
  static bool init = false;
  static mutex m;

  V res;

  m.lock();
  if (!init) {
    rng.seed(time(NULL));
    init = true;
  }

  res = dist(rng);
  m.unlock();

  return res;
}

string get_file(ifstream &in) {
  stringstream sstr;
  sstr << in.rdbuf();
  return sstr.str();
}

rapidjson::Document *utility::get_json(string key) {
  string filename = key + "_data.json";
  ifstream file(filename);

  if (!file.is_open()) {
    file.open(root_path + "/" + filename);
  }

  if (!file.is_open()) {
    throw runtime_error("Failed to find data file: " + filename);
  }

  string json_data = get_file(file);
  rapidjson::Document *doc = new rapidjson::Document();
  doc->Parse(json_data.c_str());
  if (doc->HasParseError()) {
    cout << "Error parsing json in " << filename << endl;
    cout << "Code: " << doc->GetParseError() << endl;
    cout << "Offset: " << doc->GetErrorOffset() << endl;
    cout << "JSON data: " << endl
         << json_data << endl;
    exit(-1);
  }

  if (!(doc->HasMember(key.c_str()) && (*doc)[key.c_str()].IsObject())) throw parse_error("Invalid data in " + filename + "!");
  return doc;
}

// ****************************************
// POINT ARITHMETICS
// ****************************************

vector<point> utility::cluster_points(vector<point> pos, int n, int rep, float h) {
  if (pos.empty()) return pos;
  vector<point> x(n);
  vector<point> buf;

  for (int i = 0; i < n; i++) x[i] = utility::uniform_sample(pos);

  // identify cluster points
  for (int i = 0; i < rep; i++) {
    // select a random enemy ship s
    point y = utility::uniform_sample(pos);

    // move points towards s
    for (int j = 0; j < x.size(); j++) {
      point d = y - x[j];
      float f = 50 * exp(-pow(utility::l2norm(d) / h, 2)) / (i + 1);
      x[j] = x[j] + utility::normalize_and_scale(d, f);
    }

    // join adjacent points
    bool has_duplicate;
    buf.clear();
    for (auto j = x.begin(); j != x.end(); j++) {
      has_duplicate = false;
      for (auto k = j + 1; k != x.end(); k++) {
        if (utility::l2d2(*j - *k) < 100) {
          has_duplicate = true;
          break;
        }
      }

      if (!has_duplicate) buf.push_back(*j);
    }

    swap(x, buf);
  }

  return x;
}

string utility::point2string(point p) {
  return to_string(p.x) + "x" + to_string(p.y);
}

float utility::signum(float x, float eps) {
  return (x > eps) - (x < -eps);
}

// scalar product of points a and b
float utility::scalar_mult(point a, point b) {
  return a.x * b.x + a.y * b.y;
}

// squared l2 norm of point p
float utility::l2d2(point p) {
  return scalar_mult(p, p);
}

// l2 norm of point p
float utility::l2norm(point p) {
  return sqrt(l2d2(p));
}

// angle between point p and x-axis
float utility::point_angle(point p) {
  return atan2(p.y, p.x);
}

// scale point p by factor a
point utility::scale_point(point p, float a) {
  return point(a * p.x, a * p.y);
}

// scalar projection coefficient of point a on r
float utility::sproject(point a, point r) {
  return scalar_mult(a, r) / scalar_mult(r, r);
}

float utility::angle_difference(float a, float b) {
  return modulus(a - b + M_PI, 2 * M_PI) - M_PI;
}

// shortest distance between angles a and b
float utility::angle_distance(float a, float b) {
  return fabs(angle_difference(a, b));
}

bool utility::in_triangle(point a, point b, point c, point x) {
  point p_inf(min(a.x, min(b.x, c.x)) - 1, min(a.y, min(b.y, c.y)) - 1);
  int count = 0;
  count += line_intersect(a, b, x, p_inf);
  count += line_intersect(b, c, x, p_inf);
  count += line_intersect(c, a, x, p_inf);
  return count == 1;
}

// note: points b1 and b2 in ascending order of angle vs c
float utility::triangle_relative_distance(point c, point b1, point b2, point x) {
  float a1 = utility::point_angle(b1 - c);
  float a2 = utility::point_angle(b2 - c);
  float a_sol = utility::point_angle(x - c);
  if (utility::angle_difference(a_sol, a1) > 0 && utility::angle_difference(a2, a_sol) > 0) {
    float r = utility::angle_difference(a_sol, a1) / utility::angle_difference(a2, a1);
    float lim = r * utility::l2norm(b2 - c) + (1 - r) * utility::l2norm(b1 - c);
    float dist = fmax(utility::l2norm(x - c), 0);
    return dist / lim;
  }

  return -1;
}

bool utility::line_intersect(point a, point b, point p1, point p2, point *r) {
  // ax + t dx = px + s qx
  // ay + t dy = py + s qy
  // (ay + t dy) / qy = py / qy + s
  // (ax + t dx) / qx = px / qx + s
  // (ax + t dx) / qx - (ay + t dy) / qy = px / qx - py / qy
  // qy (ax + t dx) - qx (ay + t dy) = px qy - py qx
  // qy (ax + t dx - px) = qx (ay + t dy - py)
  // t (qy dx - qx dy) = qx (ay - py) - qy (ax - px)
  // t = (qx (ay - p1y) - qy (ax - p1x)) / (qy dx - qx dy)
  // s = (ay + t dy) / qy - p1y / qy

  // check same point
  if (a == b || p2 == p1) {
    server::log("line_intersect: same point!");
    return false;
  }

  // check same gradient
  point d = b - a;
  point q = p2 - p1;
  if (q.y * d.x == q.x * d.y) {
    server::log("line_intersect: same gradient!");
    return false;
  }

  // check point of intersection
  float t = (q.x * (a.y - p1.y) - q.y * (a.x - p1.x)) / (q.y * d.x - q.x * d.y);
  float s = q.x == 0 ? (a.y + t * d.y) / q.y - p1.y / q.y : (a.x + t * d.x) / q.x - p1.x / q.x;

  if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
    if (r) *r = a + t * (b - a);
    return true;
  } else {
    return false;
  }
}

// shortest distance between p and line from a to b
float utility::dpoint2line(point p, point a, point b, point *r) {
  if (angle_distance(point_angle(p - a), point_angle(b - a)) > M_PI / 2) {
    if (r) *r = a;
    return l2norm(p - a);
  } else if (angle_distance(point_angle(p - b), point_angle(a - b)) > M_PI / 2) {
    if (r) *r = b;
    return l2norm(p - b);
  } else {
    point inter = a + sproject(p - a, b - a) * (b - a);
    if (r) *r = inter;
    return l2norm(p - inter);
  }
}

// point difference operator
point st3::operator-(const point &a, const point &b) {
  return point(a.x - b.x, a.y - b.y);
}

// point addition operator
point st3::operator+(const point &a, const point &b) {
  return point(a.x + b.x, a.y + b.y);
}

// point addition operator
point st3::operator*(const float a, const point b) {
  return point(a * b.x, a * b.y);
}

// normal ~N(m,s)
float utility::random_normal(float m, float s) {
  boost::random::normal_distribution<float> dist(m, s);
  return get_random<float>(dist);
}

// random float ~U(a,b)
float utility::random_uniform(float a, float b) {
  assert(isfinite(a) && isfinite(b));
  if (a == b) return a;
  boost::random::uniform_real_distribution<float> dist(a, b);
  return get_random<float>(dist);
}

// random uniform int in [0, limit)
unsigned int utility::random_int(int limit) {
  if (limit < 2) return 0;
  boost::random::uniform_int_distribution<> dist(0, limit - 1);
  return get_random<unsigned int>(dist);
}

int utility::angle2index(int na, float a) {
  a = modulus(a, 2 * M_PI);
  int i = a * na / (2 * M_PI);
  return max(min(i, na - 1), 0);
}

float utility::index2angle(int na, int idx) {
  return 2 * M_PI * (idx + 0.5) / (float)na;
}

int utility::int_modulus(int x, int p) {
  return x < 0 ? p + (x % p) : x % p;
}

std::vector<int> utility::sequence(int a, int b) {
  assert(a < b);
  vector<int> res(b - a);
  for (int i = a; i < b; i++) res[i] = i;
  return res;
}

std::vector<int> utility::zig_seq(int a) {
  assert(a >= 0);
  vector<int> res(2 * a + 1);
  int c = 0;
  res[0] = 0;
  for (int i = 1; i <= a; i++)
    for (int s = -1; s <= 1; s += 2) res[c++] = i * s;
  return res;
}

vector<float> utility::circular_kernel(const vector<float> &x, float s) {
  // circular kernel smooth enemy strength data
  int na = x.size();
  vector<float> buf(na);
  int r = na / 2;

  for (int i = 0; i < na; i++) {
    buf[i] = 0;
    float wsum = 0;
    for (int j = -r; j <= r; j++) {
      float w = exp(-pow(j / s, 2));
      buf[i] += w * x[utility::int_modulus(i + j + na, na)];
      wsum += w;
    }
    buf[i] /= wsum;
  }

  return buf;
}

float utility::gaussian_kernel(float x, float s) {
  return exp(-pow(x / s, 2));
}

// vector of n random floats ~U(0,1)
vector<float> utility::random_uniform_vector(int n, float a, float b) {
  boost::random::uniform_real_distribution<float> dist(a, b);
  vector<float> res(n);
  for (auto &x : res) x = get_random<float>(dist);
  return res;
}

float utility::interval_function(float a, float b, float x) {
  return (float)(x >= a && x <= b);
}

float utility::angular_hat(float x) {
  return interval_function(-M_PI, M_PI, x) * (cos(x) + 1) / 2;
}

// random point with gaussian distribution around p, sigma = r
point utility::random_point_polar(point p, float r) {
  boost::random::normal_distribution<float> dist(0, r);
  return point(p.x + get_random<float>(dist), p.y + get_random<float>(dist));
}

// p geq q both dims
bool utility::point_above(point p, point q) {
  return p.x >= q.x && p.y >= q.y;
}

// p lt q both dims
bool utility::point_below(point p, point q) {
  return p.x < q.x && p.y < q.y;
}

// a above p and below q
bool utility::point_between(point a, point p, point q) {
  return point_above(a, p) && point_below(a, q);
}

// normalized point with angle a to x-axis
point utility::normv(float a) {
  return point(cos(a), sin(a));
}

point utility::normalize_and_scale(point x, float d) {
  float a = point_angle(x);
  return point(d * cos(a), d * sin(a));
}

// vector maths
// normalize the vector
void utility::normalize_vector(vector<float> &x) {
  if (x.empty()) return;

  float sum = 0;

  for (auto y : x) sum += y;

  if (sum) {
    sum = fabs(sum);
    for (auto &y : x) y /= sum;
  } else {
    for (auto &y : x) y = 0;
  }
}

// l2norm of the vector
float utility::vl2norm(vector<float> const &x) {
  float ss = 0;
  for (auto &z : x) ss += z * z;
  return sqrt(ss);
}

// vector difference
vector<float> utility::vdiff(vector<float> const &a, vector<float> const &b) {
  if (a.size() != b.size()) {
    throw logical_error("vdiff: dimension mismatch");
  }

  vector<float> res = a;
  for (int i = 0; i < res.size(); i++) {
    res[i] -= b[i];
  }

  return res;
}

// vector addition
vector<float> utility::vadd(vector<float> const &a, vector<float> const &b) {
  if (a.size() != b.size()) {
    throw logical_error("vdiff: dimension mismatch");
  }

  vector<float> res = a;
  for (int i = 0; i < res.size(); i++) {
    res[i] += b[i];
  }

  return res;
}

// rescale vector a by s
vector<float> utility::vscale(vector<float> a, float s) {
  for (auto &x : a) x *= s;
  return a;
}

// sigmoid function: approaches -s below -s and s above s, like
// identity function between
float utility::sigmoid(float x, float s) {
  return s * atan(x / s) / (M_PI / 2);
}

float utility::linsig(float x) {
  return fmax(fmin(x, 1), 0);
}

// generate a vector of n different colors, which are also different
// from black (background) and grey (neutral solar) colors
vector<sint> utility::different_colors(int n) {
  int rep = 40;
  int ncheck = n + 4;
  vector<vector<float> > buf(ncheck);

  for (auto &x : buf) {
    x = utility::random_uniform_vector(3);
  }

  buf[n] = {0, 0, 0};                                                   // space color
  buf[n + 1] = {150 / (float)256, 150 / (float)256, 150 / (float)256};  // neutral color
  buf[n + 2] = {1, 0, 0};                                               // enemy color
  buf[n + 3] = {1, 1, 1};                                               // white: symbol background

  for (int i = 0; i < rep; i++) {
    for (int j = 0; j < n; j++) {
      float dmin = 10;
      int t = -1;
      for (int k = 0; k < ncheck; k++) {
        if (j != k) {
          float d = utility::vl2norm(utility::vdiff(buf[j], buf[k]));
          if (d < dmin) {
            dmin = d;
            t = k;
          }
        }
      }

      if (t > -1) {
        // move point j away from point t
        vector<float> delta = utility::vdiff(buf[j], buf[t]);
        buf[j] = utility::vadd(buf[j], utility::vscale(delta, 0.05));
        for (int k = 0; k < 3; k++) buf[j][k] = fmin(fmax(buf[j][k], 0), 1);
      }
    }
  }

  // compile result
  vector<sint> res(n);
  for (int i = 0; i < n; i++) {
    sint red = buf[i][0] * 255;
    sint green = buf[i][1] * 255;
    sint blue = buf[i][2] * 255;
    res[i] = (red << 24) | (green << 16) | (blue << 8) | 0xff;
  }

  return res;
}

point utility::p2solve(float a, float b, float c) {
  // a x^2 + b x + c = 0
  // x^2 + b/a x + c/a = 0
  // (x + b/(2a))^2 + c/a = (b/(2a))^2
  // x = -b/(2a) +/- sqrt((b/(2a))^2 - c/a)
  float s1 = -b / (2 * a) - sqrt(pow(b / (2 * a), 2) - c / a);
  float s2 = -b / (2 * a) + sqrt(pow(b / (2 * a), 2) - c / a);
  return point(s1, s2);
}

// return x mod p, presuming p > 0
float utility::modulus(float x, float p) {
  int num = floor(x / p);
  return x - num * p;
}

// format a float as low prec. string
string utility::format_float(float x) {
  stringstream stream;
  stream << fixed << setprecision(2) << x;
  return stream.str();
}

float utility::mass2area(float m) {
  return pow(m, 2 / (float)3);
}

float utility::safe_inv(float x) {
  return x ? 1 / x : INFINITY;
}

// output point x to stream ss
ostream &st3::operator<<(ostream &ss, point const &x) {
  ss << "(" << x.x << ", " << x.y << ")";
  return ss;
}
