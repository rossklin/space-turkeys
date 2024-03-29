#include "types.hpp"

#include <chrono>  // chrono::system_clock
#include <ctime>   // localtime
#include <fstream>
#include <iomanip>  // put_time
#include <iostream>
#include <mutex>
#include <sstream>

#include "server_handler.hpp"
#include "utility.hpp"
#include "waypoint.hpp"

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
    keywords::key_gases};

const vector<string> keywords::development = {
    keywords::key_population,
    keywords::key_research,
    keywords::key_shipyard,
    keywords::key_defense};

const vector<string> keywords::solar_modifier = {
    // keywords::key_agriculture,
    keywords::key_research,
    keywords::key_shipyard,
    keywords::key_defense,
    // keywords::key_medicine,
    keywords::key_population};

// // make a source symbol with type t and id i
// idtype identifier::make(class_t t, idtype i) {
//   stringstream s;
//   s << t << ":" << i;
//   return s.str();
// }

// // make a source symbol with type t and string id k
// idtype identifier::make(class_t t, string k) {
//   stringstream s;
//   s << t << ":" << k;
//   return s.str();
// }

// // get the type of source symbol s
// class_t identifier::get_type(idtype s) {
//   size_t split = s.find(':');
//   return s.substr(0, split);
// }

// // get the owner id of waypoint symbol string id v
// idtype identifier::get_multid_owner(idtype v) {
//   string x = get_multid_owner_symbol(v);
//   try {
//     return stoi(x);
//   } catch (...) {
//     throw classified_error("get multid owner: invalid id from " + v + ": " + x);
//   }
// }

// // get the owner id of waypoint symbol string id v
// string identifier::get_multid_owner_symbol(idtype v) {
//   size_t split1 = v.find(':');
//   size_t split2 = v.find('#');
//   return v.substr(split1 + 1, split2 - split1 - 1);
// }

// // get the owner id of waypoint symbol string id v
// idtype identifier::get_multid_index(idtype v) {
//   string x = get_multid_index_symbol(v);
//   try {
//     return stoi(x);
//   } catch (...) {
//     throw classified_error("get multid index: invalid id from " + v + ": " + x);
//   }
// }

// // get the owner id of waypoint symbol string id v
// string identifier::get_multid_index_symbol(idtype v) {
//   size_t split = v.find('#');
//   return v.substr(split + 1);
// }

// idtype identifier::make_waypoint_id(idtype owner, idtype id) {
//   return identifier::make(waypoint::class_id, to_string(owner) + "#" + to_string(id));
// }

// id_pair::id_pair(idtype x, idtype y) {
//   a = x;
//   b = y;
// }

// bool st3::operator<(const id_pair &x, const id_pair &y) {
//   return hash<string>{}(x.a) ^ hash<string>{}(x.b) < hash<string>{}(y.a) ^ hash<string>{}(y.b);
// }
