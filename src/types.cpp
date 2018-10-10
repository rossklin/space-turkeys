#include <sstream>
#include <iostream>
#include <mutex>
#include <fstream>
#include <chrono>  // chrono::system_clock
#include <ctime>   // localtime
#include <iomanip> // put_time

#include "types.h"
#include "waypoint.h"
#include "utility.h"
#include "server_handler.h"

using namespace std;
using namespace st3;

// #define DEBUG true

bool server::silent_mode = false;
const float st3::accuracy_distance_norm = 30;
const float st3::evasion_mass_norm = 10;

classified_error::classified_error(string v, string s) : runtime_error(v) {
  severity = s;
}

network_error::network_error(string v, string s) : classified_error(v, s) {}
logical_error::logical_error(string v, string s) : classified_error(v, s) {}
player_error::player_error(string v, string s) : classified_error(v, s) {}
parse_error::parse_error(string v, string s) : classified_error(v, s) {}

string current_time_and_date() {
  auto now = chrono::system_clock::now();
  auto in_time_t = chrono::system_clock::to_time_t(now);

  stringstream ss;
  ss << put_time(localtime(&in_time_t), "%Y-%m-%d %X");
  return ss.str();
}

void server::log(string v, string severity) {
  static mutex m;
  string log_file = "server.log";

  m.lock();
  fstream f(log_file, ios::app);

  if (!f.is_open()) {
    cerr << "Error: " << v << endl;
    cerr << "Failed to open log file!" << endl;
    exit(-1);
  }

  f << current_time_and_date() << ": " << severity << ": " << v << endl;
  f.close();
  
  m.unlock();
}

void server::output(string v, bool force) {
  static mutex m;

  if (silent_mode) return;
  
#ifdef DEBUG
  force = true;
#endif

  if (force) {
    m.lock();
    cout << v << endl;
    m.unlock();
  }
}

const vector<string> keywords::resource = {
  keywords::key_metals,
  keywords::key_organics,
  keywords::key_gases
};

const vector<string> keywords::sector = {
  keywords::key_research,
  keywords::key_culture,
  keywords::key_military,
  keywords::key_mining,
  keywords::key_development,
  keywords::key_medicine,
  keywords::key_ecology
};

const vector<string> keywords::governor = {
  keywords::key_research,
  keywords::key_culture,
  keywords::key_military,
  keywords::key_mining,
  keywords::key_development
};

// make a source symbol with type t and id i
combid identifier::make(class_t t, idtype i){
  stringstream s;
  s << t << ":" << i;
  return s.str();
}

// make a source symbol with type t and string id k
combid identifier::make(class_t t, string k){
  stringstream s;
  s << t << ":" << k;
  return s.str();
}

// get the type of source symbol s
class_t identifier::get_type(combid s){
  size_t split = s.find(':');
  return s.substr(0, split);
}

// get the owner id of waypoint symbol string id v
idtype identifier::get_multid_owner(combid v){
  string x = get_multid_owner_symbol(v);
  try{
    return stoi(x);
  }catch(...){
    throw classified_error("get multid owner: invalid id from " + v + ": " + x);
  }
}

// get the owner id of waypoint symbol string id v
string identifier::get_multid_owner_symbol(combid v){
  size_t split1 = v.find(':');
  size_t split2 = v.find('#');
  return v.substr(split1 + 1, split2 - split1 - 1);
}

// get the owner id of waypoint symbol string id v
idtype identifier::get_multid_index(combid v){
  string x = get_multid_index_symbol(v);
  try{
    return stoi(x);
  }catch(...){
    throw classified_error("get multid index: invalid id from " + v + ": " + x);
  }
}

// get the owner id of waypoint symbol string id v
string identifier::get_multid_index_symbol(combid v){
  size_t split = v.find('#');
  return v.substr(split + 1);
}

combid identifier::make_waypoint_id(idtype owner, idtype id){
  return identifier::make(waypoint::class_id, to_string(owner) + "#" + to_string(id));
}

id_pair::id_pair(combid x, combid y) {
  a = x;
  b = y;
}

bool st3::operator < (const id_pair &x, const id_pair &y) {
  return hash<string>{}(x.a)^hash<string>{}(x.b) < hash<string>{}(y.a)^hash<string>{}(y.b);
}

pair<int, int> terrain_object::intersects_with(terrain_object obj, float r) {
  r *= 0.5;
  for (int i = 0; i < border.size(); i++) {
    point p1 = get_vertice(i, r);
    point p2 = get_vertice(i+1, r);
	
    for (int j = 0; j < obj.border.size(); j++) {
      point q1 = obj.get_vertice(j, r);
      point q2 = obj.get_vertice(j+1, r);

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
  
int terrain_object::triangle(point p, float rad) {
  float a_sol = utility::point_angle(p - center);
  for (int j = 0; j < border.size(); j++) {
    if (utility::in_triangle(center, get_vertice(j, rad), get_vertice(j+1, rad), p)) return j;
  }

  return -1;
}

point terrain_object::closest_exit(point p, float r) const {
  float dmin = INFINITY;
  point res;

  for (int i = 0; i < border.size(); i++) {
    point a = get_vertice(i, r);
    point b = get_vertice(i+1, r);
    point x;
    float d = utility::dpoint2line(p, a, b, &x);
    if (d < dmin) {
      dmin = d;
      res = x;
    }
  }

  return res;
}
